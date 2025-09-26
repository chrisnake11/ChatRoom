# ChatRoom - 多服务器架构聊天室项目

## 原始项目地址：https://www.bilibili.com/video/BV1k2421K7ZB/
## 作者博客：https://llfc.club/

自学用。这是一个基于C++、Qt和Node.js的分布式聊天室服务器，在llfc的B站视频教程的服务器和客户端代码框架基础上，添加好友申请、离线消息缓存、本地数据缓存等功能，并计划后续添加文件传输、音视频通话等功能。

## 项目架构

```
ChatRoom/
├── ChatRoom2/          # Qt客户端
├── GateServer1/        # 网关服务器 (C++)
├── ChatServer1/        # 聊天服务器1 (C++)  
├── ChatServer2/        # 聊天服务器2 (C++)
├── StatusServer/       # 状态服务器 (C++)
├── VarifyServer/       # 验证服务器 (Node.js)
└── chatroom.sql        # 数据库脚本
```

# TODO 

## UI逻辑bug
- [x] 处理联系人界面，联系人名称异常（显示为当前用户而不是对方）。
- [x] 处理非正常头像后，添加默认头像。
- [x] 处理Chat_Content切换Message联系人后，不清空直接叠加的bug。

## 重构
- [x] 重构MysqlManager和MysqlDao类。
- [x] 将Redis，ChatMessage从字符串变为数组缓存。
- [x] 增加确保Redis和Mysql的缓存一致性，读取Redis自动同步MySQL数据。

## 发送聊天消息
- [x] 客户端：
- [x] 服务端：发送聊天消息，提交Message信息，并修改friend_relationship表
- [x] 实现GRPC消息通信
- [ ] 实现消息提醒功能 

## 添加好友
- [x] 添加好友界面
- [x] 搜索好友
- [x] 申请界面
- [x] 发送申请
- [x] 同意/拒绝申请
- [ ] 查询ChatServer，通知对方客户端更新。

## 实现离线缓存

使用 Redis 作为离线消息的快速存取缓存，使用 MySQL 作为离线消息的持久化存储
+ 消息发送时（对方离线）：
1. 直接存储到 Redis 的离线消息队列中
2. 返回发送成功给客户端
3. 异步将消息写入 MySQL 作为持久化备份
+ 用户登录时：
1. ChatServer 从 Redis 拉取该用户的离线消息
2. 推送完成后，标记或删除 Redis 中的消息
3. MySQL 中的记录作为历史消息存档


## 实现本地数据缓存

使用 SQLite 作为客户端的本地缓存数据库，SQLite -> Redis -> MySQL

## 实现视频/图片文件上传功能

## 添加日志系统

## 实现FFmpeg视频播放功能

## 实现视频/音频通话功能

## 实现远端操控功能

## 实现群聊功能

