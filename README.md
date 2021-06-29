# YYWebServer

## 一个简单的C++ Web 服务器

本项目是C++11编写的 Linux Web服务器。

#### 实现了：


* 支持HTTP 1.0 ／ HTTP1.1 长连接
* 支持 GET请求，静态资源访问
* Reactor 事件处理模式，Epoll边缘触发，非阻塞IO，主线程负责接收连接，子线程负责业务处理（计算+IO）
* 使用多线程充分利用CPU，半同步半反应堆多线程池实现线程复用，节省创建销毁的开销 
* 加入基于使用优先队列实现的小根堆的计时器及时清除不活跃的连接
* 基于状态机解析HTTP请求
* 支持谁创建谁销毁设计原则


#### 环境

* ubuntu16.04
* gcc7.4