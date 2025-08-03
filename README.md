# ChatRoom - 多服务器架构聊天室项目

这是一个基于C++、Qt和Node.js的分布式聊天室系统，采用微服务架构设计。

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