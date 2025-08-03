# ChatRoom - 多服务器架构聊天室项目

## 原始项目地址：https://www.bilibili.com/video/BV1k2421K7ZB/
## 作者博客：https://llfc.club/

自学用。这是一个基于C++、Qt和Node.js的分布式聊天室系统。

## 项目架构

```
ChatRoom/
├── ChatRoom2/          # Qt客户端
├── GateServer1/        # 网关服务器 (C++)
├── ChatServer1/        # 聊天服务器 (C++)  
├── StatusServer/       # 状态服务器 (C++)
├── VarifyServer/       # 验证服务器 (Node.js)
└── chatroom.sql        # 数据库脚本
```