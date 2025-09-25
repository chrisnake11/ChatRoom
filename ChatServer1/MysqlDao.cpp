#include "MysqlDao.h"
#include <chrono>
#include "MysqlManager.h"
#include "Utils.h"

MysqlDao::MysqlDao()
{
}

MysqlDao::~MysqlDao()
{
}

std::unique_ptr<UserInfo> MysqlDao::getUserInfo(int uid, sql::Connection* conn)
{
	// 从数据库查询用户基本信息
    std::unique_ptr<UserInfo> user_info = nullptr;

    std::unique_ptr<sql::PreparedStatement> pstmt(
        conn->prepareStatement("SELECT * from user_info where uid = ?"));
    pstmt->setInt(1, uid);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next()) {
        user_info = std::make_unique<UserInfo>();
        user_info->uid = res->getInt("uid");
        user_info->username = res->getString("username");
        user_info->email = res->getString("email");
        user_info->passwd = res->getString("passwd");
        user_info->nickname = res->getString("nickname");
        user_info->phone = res->getString("phone");
        user_info->address = res->getString("address");
        user_info->avatar = res->getString("avatar");
        user_info->gender = res->getInt("gender");
        user_info->birthday = res->getString("birthday");
        user_info->sign = res->getString("personal_signature");
        user_info->online_status = res->getInt("online_status");
        user_info->last_login = res->getString("last_login");
        user_info->register_time = res->getString("register_time");
    }
    return user_info;
}

int MysqlDao::updateLoginStatus(int uid, int status, const std::string& last_login, sql::Connection* conn) {
    if (!last_login.empty()) {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->prepareStatement("UPDATE user_info SET online_status = ?, last_login = ? WHERE uid = ?"));
        pstmt->setInt(1, status);
        pstmt->setString(2, last_login);
        pstmt->setInt(3, uid);
        return pstmt->executeUpdate();
    }
    else {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->prepareStatement("UPDATE user_info SET online_status = ? WHERE uid = ?"));
        pstmt->setInt(1, status);
        pstmt->setInt(2, uid);
        return pstmt->executeUpdate();
    }
}

std::unique_ptr<std::vector<MessageInfo>> MysqlDao::getMessageList(int uid, sql::Connection* conn) {
    std::string query_str = "SELECT fr.friend_id, fr.last_message_id, fr.unread_count, msg.content, msg.timestamp, ui.nickname, ui.avatar "
        "FROM friend_relationship fr LEFT JOIN message msg "
        "ON fr.last_message_id = msg.message_id "
        "LEFT JOIN user_info ui "
        "ON fr.friend_id = ui.id "
        "WHERE fr.user_id = ? and fr.friend_status = 0";
    std::unique_ptr<sql::PreparedStatement> pstmt(
        conn->prepareStatement(query_str));
    pstmt->setInt(1, uid);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    std::unique_ptr<std::vector<MessageInfo>> message_list = std::make_unique<std::vector<MessageInfo>>();
    while (res->next()) {
        // 将结果添加到message_list中
        MessageInfo message_item;
        message_item.uid = res->getInt("friend_id");
        message_item.nickname = res->getString("nickname");
        message_item.avatar = res->getString("avatar");
        message_item.message = res->getString("content");
        message_item.last_message_time = res->getString("timestamp");
        message_item.unread_count = res->getInt("unread_count");
        message_list->push_back(message_item);
    }
    return std::move(message_list);
}

std::unique_ptr<std::vector<ContactInfo>> MysqlDao::getContactList(int uid, sql::Connection* conn) {
    std::unique_ptr<std::vector<ContactInfo>> contact_list = std::make_unique<std::vector<ContactInfo>>();
    std::string query_str = "SELECT fr.friend_id, ui.username, ui.nickname, ui.avatar, ui.personal_signature, ui.online_status "
        "FROM friend_relationship fr LEFT JOIN user_info ui "
        "ON fr.friend_id = ui.id "
        "WHERE fr.user_id = ? and fr.friend_status = 0";
    std::unique_ptr<sql::PreparedStatement> pstmt(
        conn->prepareStatement(query_str));
    pstmt->setInt(1, uid);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    while (res->next()) {
        ContactInfo contact_item;
        contact_item.uid = res->getInt("friend_id");
        contact_item.username = res->getString("username");
        contact_item.nickname = res->getString("nickname");
        contact_item.avatar = res->getString("avatar");
        contact_item.sign = res->getString("personal_signature");
        contact_item.online_status = res->getInt("online_status");
        contact_list->push_back(contact_item);
    }
    return std::move(contact_list);

}

std::unique_ptr<std::vector<ChatMessageInfo>> MysqlDao::getChatMessageList(const int& uid, const int& friend_uid, const int& last_message_id, sql::Connection* conn)
{
    std::unique_ptr<std::vector<ChatMessageInfo>> message_list = std::make_unique<std::vector<ChatMessageInfo>>();
    // 获取大于last_message_id的聊天消息
    std::string query_str = "SELECT message_id, sender_id, receiver_id, content as message, timestamp, type "
        "FROM message "
        "WHERE ((sender_id = ? AND receiver_id = ?) OR (sender_id = ? AND receiver_id = ?)) AND message_id > ? "
        "ORDER BY message_id ASC";
    std::unique_ptr<sql::PreparedStatement> pstmt(
        conn->prepareStatement(query_str));
    pstmt->setInt(1, uid);
    pstmt->setInt(2, friend_uid);
    pstmt->setInt(3, friend_uid);
    pstmt->setInt(4, uid);
    pstmt->setInt(5, last_message_id);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    while (res->next()) {
        // 解析数据
        ChatMessageInfo message_item;
        message_item.message_id = res->getInt("message_id");
        message_item.sender_id = res->getInt("sender_id");
        message_item.receiver_id = res->getInt("receiver_id");
        message_item.message = res->getString("message");
        message_item.message_time = res->getString("timestamp");
        message_item.message_type = res->getString("type");
        // 添加到结果列表
        message_list->push_back(message_item);
    }
    return std::move(message_list);

}

int MysqlDao::insertChatMessage(ChatMessageInfo& message, sql::Connection* conn) {
    std::string query_str =
        "INSERT INTO message (sender_id, receiver_id, content, timestamp, type) "
        "VALUES (?, ?, ?, ?, ?)";
    std::unique_ptr<sql::PreparedStatement> pstmt(
        conn->prepareStatement(query_str));
    pstmt->setInt(1, message.sender_id);
    pstmt->setInt(2, message.receiver_id);
    pstmt->setString(3, message.message);
    pstmt->setDateTime(4, message.message_time);
    pstmt->setString(5, message.message_type);
    return pstmt->executeUpdate();
}

int MysqlDao::getLastMessageId(const int& sender_id, const int& receiver_id, sql::Connection* conn) {
    std::string query_str =
        "SELECT message_id FROM message "
        "WHERE sender_id = ? AND receiver_id = ? "
        "ORDER BY message_id DESC LIMIT 1"; // 取最新插入的消息
    std::unique_ptr<sql::PreparedStatement> pstmt(
        conn->prepareStatement(query_str));
    pstmt->setInt(1, sender_id);
    pstmt->setInt(2, receiver_id);
    std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());
    if (rs->next()) {
        return rs->getInt("message_id");
    }
    return -1;
}

int MysqlDao::updateFriendRelationshipMessage(const int& message_id, const int& sender_id, const int& receiver_id, sql::Connection* conn) {
    std::string query_str =
        "UPDATE friend_relationship "
        "SET last_message_id = ?, unread_count = unread_count + 1 "
        "WHERE user_id = ? AND friend_id = ?";
    std::unique_ptr<sql::PreparedStatement> pstmt(
        conn->prepareStatement(query_str));
    pstmt->setInt(1, message_id);
    pstmt->setInt(2, sender_id);
    pstmt->setInt(3, receiver_id);

    return pstmt->executeUpdate();
}

std::unique_ptr<FriendRelationship> MysqlDao::getFriendRelationship(std::unique_ptr<FriendRelationship> fr, sql::Connection* conn) {
    std::unique_ptr<sql::PreparedStatement> pstmt(
        conn->prepareStatement("SELECT * FROM friend_relationship WHERE uid_min = ? AND uid_max = ?"));
    pstmt->setInt(1, std::min(fr->uid, fr->friend_uid));
    pstmt->setInt(2, std::max(fr->uid, fr->friend_uid));
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if(res->next()) { 
        fr->id = res->getInt("id");
        fr->friend_status = res->getInt("friend_status");
        fr->create_time = res->getString("create_time");
        fr->last_message_id = res->getInt("last_message_id");
        fr->unread_count = res->getInt("unread_count");
    }
    return std::move(fr);
}


int MysqlDao::updateFriendRelationship(std::unique_ptr<FriendRelationship> fr, sql::Connection* conn) {
    std::string query_str = "UPDATE friend_relationship "
        "SET friend_status = ? create_time = ? last_message_id = ?, unread_count = ? "
        "WHERE uid_min = ? AND uid_max = ? ";
    std::unique_ptr<sql::PreparedStatement> pstmt(
        conn->prepareStatement("UPDATE friend_relationship SET friend_status = ? create_time = ? last_message_id = ? unread_count = ? WHERE uid_min = ? AND uid_max = ?"));
    pstmt->setInt(1, fr->friend_status);
    pstmt->setString(2, fr->create_time);      // 假设是 string
    pstmt->setInt(3, fr->last_message_id);
    pstmt->setInt(4, fr->unread_count);
    pstmt->setInt(5, std::min(fr->uid, fr->friend_uid));
    pstmt->setInt(6, std::max(fr->uid, fr->friend_uid));
    return pstmt->executeUpdate();
}

std::unique_ptr<std::vector<SearchFriendInfo>> MysqlDao::searchFriendList(const std::string& friend_name, sql::Connection* conn)
{
    std::string query_str = "SELECT uid, username, nickname, avatar "
        "FROM user_info "
		"WHERE username LIKE ? OR nickname LIKE ? ";
    std::string like_pattern = "%" + friend_name + "%";
    std::unique_ptr<sql::PreparedStatement> pstmt(
        conn->prepareStatement(query_str));
    pstmt->setString(1, like_pattern);
    pstmt->setString(2, like_pattern);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
	std::unique_ptr<std::vector<SearchFriendInfo>> searchFriendList = 
        std::make_unique<std::vector<SearchFriendInfo>>();
    while (res->next()) {
        SearchFriendInfo searchFriendInfo;
        searchFriendInfo.uid = res->getInt("uid");
        searchFriendInfo.username = res->getString("username");
        searchFriendInfo.nickname = res->getString("nickname");
        searchFriendInfo.avatar = res->getString("avatar");
        searchFriendList->push_back(searchFriendInfo);
    }

    if (searchFriendList->empty()) {
        return nullptr;
    }
    return std::move(searchFriendList);
}

bool MysqlDao::existFriendRequest(const int& uid, const int& friend_uid, sql::Connection* conn)
{
    std::string query_str = "SELECT COUNT(*) as request_count "
        "FROM friend_request "
		"WHERE (sender_id = ? AND receiver_id = ?) OR (sender_id = ? AND receiver_id = ?)";
    std::unique_ptr<sql::PreparedStatement> pstmt(
		conn->prepareStatement(query_str));
    pstmt->setInt(1, uid);
    pstmt->setInt(2, friend_uid);
    pstmt->setInt(3, friend_uid);
    pstmt->setInt(4, uid);
	std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next()) {
        int count = res->getInt("request_count");
        return count > 0;
	}
    return false;
}

int MysqlDao::insertFriendRequest(const int& uid, const int& friend_uid, sql::Connection* conn)
{
    std::string query_str =         "INSERT INTO friend_request (sender_id, receiver_id, create_time, status) "
		"VALUES (?, ?, ?, ?, ?)";
    std::string datetime_str = getDateTimeStr();
    std::unique_ptr<sql::PreparedStatement> pstmt(
        conn->prepareStatement(query_str));
    pstmt->setInt(1, uid);
    pstmt->setInt(2, friend_uid);
    pstmt->setString(3, datetime_str);
    pstmt->setInt(4, 0); // 0表示未处理，1表示已接受，2表示已拒绝
	return pstmt->executeUpdate();
}

std::unique_ptr<std::vector<AddFriendInfo>> MysqlDao::getFriendRequestList(const int& uid, sql::Connection* conn)
{
    std::string query_str = "SELECT fr.sender_id, fr.receiver_id, create_time, status, message, ui.avatar, ui.username, ui.nickname "
        "FROM friend_request fr LEFT JOIN user_info ui "
        "ON fr.receiver_id = ui.id "
        "WHERE fr.sender_id = ?";
    std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query_str));
    pstmt->setInt(1, uid);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

    std::unique_ptr<std::vector<AddFriendInfo>> addFriendList = std::make_unique<std::vector<AddFriendInfo>>();

    while(res->next()) {
        AddFriendInfo addFriendInfo;
        addFriendInfo.sender_id = res->getInt("sender_id");
        addFriendInfo.receiver_id = res->getInt("receiver_id");
        addFriendInfo.request_time = res->getString("create_time");
        addFriendInfo.status = res->getInt("status");
        addFriendInfo.avatar = res->getString("avatar");
        addFriendInfo.username = res->getString("username");
        addFriendInfo.nickname = res->getString("nickname");
        addFriendList->push_back(addFriendInfo);
    }
    return addFriendList;
}

std::unique_ptr<std::vector<AddFriendInfo>> MysqlDao::getFriendResponseList(const int& uid, sql::Connection* conn)
{
    std::string query_str = "SELECT fr.sender_id, fr.receiver_id, create_time, status, message, ui.avatar, ui.username, ui.nickname "
        "FROM friend_request fr LEFT JOIN user_info ui "
        "ON fr.sender_id = ui.id "
        "WHERE fr.receiver_id = ?";
    std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query_str));
    pstmt->setInt(1, uid);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

    std::unique_ptr<std::vector<AddFriendInfo>> addFriendList = std::make_unique<std::vector<AddFriendInfo>>();

    while (res->next()) {
        AddFriendInfo addFriendInfo;
        addFriendInfo.sender_id = res->getInt("sender_id");
        addFriendInfo.receiver_id = res->getInt("receiver_id");
        addFriendInfo.request_time = res->getString("create_time");
        addFriendInfo.status = res->getInt("status");
        addFriendInfo.avatar = res->getString("avatar");
        addFriendInfo.username = res->getString("username");
        addFriendInfo.nickname = res->getString("nickname");
        addFriendList->push_back(addFriendInfo);
    }
    return addFriendList;
}
