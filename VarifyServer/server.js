const grpc = require('@grpc/grpc-js');
const message_proto = require('./proto.js'); // 引入proto文件
const const_module = require('./const.js'); // 引入常量模块
const emailModule = require('./email.js'); // 引入邮件模块
const { v4: uuidv4 } = require('uuid'); // 引入uuid模块生成唯一的验证码
const redis_module = require('./redis.js'); // 引入Redis模块

// 验证码请求函数
async function GetVarifyCode(call, callback) {
    // 答应请求的email
    console.log("email is ", call.request.email)
    try{
        let query_res = await redis_module.getRedis(const_module.code_prefix + call.request.email);
        console.log("query res is ", query_res)
        let uniqueId = query_res;

        // 如果Redis中没有验证码，生成一个新的验证码
        if(query_res == null){
            uniqueId = uuidv4();
            if(uniqueId.length > 4){
                uniqueId = uniqueId.substring(0, 4); // 截取前四位作为验证码
                console.log("uniqueId is ", uniqueId)
            }
            // 设置Redis中的验证码
            let bres = await redis_module.setRedisExpire(const_module.code_prefix + call.request.email, uniqueId, 180); // 设置验证码到Redis，3分钟过期
            if(bres === null){
                // 如果设置Redis失败，返回错误
                callback(null, { email:  call.request.email,
                    error: const_module.Errors.RedisErr
                });
                return;
            }
        }
        
        // 如果Redis中存在验证码，直接使用
        console.log("uniqueId is ", uniqueId)
        let text_str =  '您的验证码为'+ uniqueId +'请三分钟内完成注册'
        //发送邮件内容
        let mailOptions = {
            from: '2500517199@qq.com', // 发送方邮箱地址, 授权码对应的邮箱
            to: call.request.email, // 接收方邮箱地址
            subject: '验证码',
            text: text_str, // 邮件内容
        };
        // 引入邮件模块，发送给request.email
        let send_res = await emailModule.SendMail(mailOptions);
        
        // 如果发送邮件失败，返回错误
        if(send_res === null){
            console.log("send mail error")
            callback(null, { email:  call.request.email,
                error: const_module.Errors.Exception
            });
            return;
        }

        // 处理发送结果
        console.log("send res is ", send_res)
        // 将验证码存入Redis
        callback(null, { email:  call.request.email,
            error:const_module.Errors.Success
        }); 
    }catch(error){
        console.log("catch error is ", error)
        callback(null, { email:  call.request.email,
            error:const_module.Errors.Exception
        }); 
    }
}

function main() {
    var server = new grpc.Server()
    // 添加GetVarifyCode接口到grpc服务
    server.addService(message_proto.VarifyService.service, { GetVarifyCode: GetVarifyCode })
    // 绑定grpc服务到指定端口，监听grpc请求
    // grpc.ServerCredentials.createInsecure() 表示不使用安全连接
    server.bindAsync('0.0.0.0:50051', grpc.ServerCredentials.createInsecure(), () => {
        console.log('grpc server started')        
    })
}

main()