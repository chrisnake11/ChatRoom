#pragma once
#include <vector>
#include <memory>
#include "Data.h"
#include <jdbc/mysql_connection.h>

// 封装与MySQL数据库的操作
class MysqlDao {
public:
    MysqlDao();
    ~MysqlDao();

    // Dao层接收sql::Connection裸指针，操作不涉及指针delete。
	std::unique_ptr<UserInfo> getUserInfo(int uid, sql::Connection* conn);

    // 更新用户登录状态和最后登录时间，如果登陆时间为空，则不更新时间。
    int updateLoginStatus(int uid, int status, const std::string& last_login, sql::Connection* conn);

    // 获取用户所有的聊天消息，以数组的形式返回
    std::unique_ptr<std::vector<MessageInfo>> getMessageList(int uid, sql::Connection* conn);
    // 获取联系人列表，以数组的形式返回
    std::unique_ptr<std::vector<ContactInfo>> getContactList(int uid, sql::Connection* conn);
    // 获取聊天消息列表，以数组的形式返回
    std::unique_ptr<std::vector<ChatMessageInfo>> getChatMessageList(const int& uid, const int& friend_uid, const int& last_message_id, sql::Connection* conn);

    // 插入聊天消息
    int insertChatMessage(ChatMessageInfo& message, sql::Connection* conn);
       
    // 获取最后消息的id
    int getLastMessageId(const int& sender_id, const int& receiver_id, sql::Connection* conn);

    // 更新好友的未读消息和未读数量
    int updateFriendRelationshipMessage(const int& message_id, const int& sender_id, const int& receiver_id, sql::Connection* conn);

    // 获取好友关系
    std::unique_ptr<FriendRelationship> getFriendRelationship(std::unique_ptr<FriendRelationship> fr, sql::Connection* conn);

    // 更新好友关系
    int updateFriendRelationship(std::unique_ptr<FriendRelationship> fr, sql::Connection* conn);

	std::unique_ptr<std::vector<SearchFriendInfo>> searchFriendList(const std::string& friend_name, sql::Connection* conn);

	bool existFriendRequest(const int& uid, const int& friend_uid, sql::Connection* conn);

	int insertFriendRequest(const int& uid, const int& friend_uid, sql::Connection* conn);

    std::unique_ptr<std::vector<AddFriendInfo>> getFriendRequestList(const int& uid, sql::Connection* conn);
    std::unique_ptr<std::vector<AddFriendInfo>> getFriendResponseList(const int& uid, sql::Connection* conn);
};
