OpenGL ES 共享上下文时，可以共享哪些资源？

![](https://ask.qcloudimg.com/http-save/7399394/qljk1i8ai.gif)

OpenGL ES 共享上下文实现多线程渲染

**EGL 是 OpenGL ES 和本地窗口系统（Native Window System）之间的通信接口**，它的主要作用：

*   与设备的原生窗口系统通信；
*   查询绘图表面的可用类型和配置；
*   创建绘图表面；
*   在OpenGL ES 和其他图形渲染API之间同步渲染；
*   管理纹理贴图等渲染资源。

**OpenGL ES 的平台无关性正是借助 EGL 实现的，EGL 屏蔽了不同平台的差异（Apple 提供了自己的 EGL API 的 iOS 实现，自称 EAGL）。** 

**本地窗口相关的 API 提供了访问本地窗口系统的接口，而 EGL 可以创建渲染表面 EGLSurface ，同时提供了图形渲染上下文 EGLContext，用来进行状态管理，接下来 OpenGL ES 就可以在这个渲染表面上绘制。** 

![](https://ask.qcloudimg.com/http-save/7399394/pt73e0lel1.jpeg?imageView2/2/w/1620)

egl、opengles 和设备之间的关系

图片中：

*   Display(EGLDisplay) 是对实际显示设备的抽象；
*   Surface（EGLSurface）是对用来存储图像的内存区域 FrameBuffer 的抽象，包括 Color Buffer（颜色缓冲区）， Stencil Buffer（模板缓冲区） ，Depth Buffer（深度缓冲区）；
*   Context (EGLContext) 存储 OpenGL ES 绘图的一些状态信息；

**在 Android 平台上开发 OpenGL ES 应用时，类 GLSurfaceView 已经为我们提供了对 Display , Surface , Context 的管理，即 GLSurfaceView 内部实现了对 EGL 的封装，可以很方便地利用接口 GLSurfaceView.Renderer 的实现，使用 OpenGL ES API 进行渲染绘制，很大程度上提升了 OpenGLES 开发的便利性。** 

当然我们也可以自己实现对 EGL 的封装，本文就是在 Native 层对 EGL 进行封装，不借助于 GLSurfaceView ，实现图片后台渲染，利用 [GPU](https://cloud.tencent.com/product/gpu?from=10680) 完成对图像的高效处理。

关于 EGL 更详细的使用结束，可以参考系列文章中的 OpenGL ES 3.0 开发（六）：EGL

共享上下文时，可以跨线程共享哪些资源？这个是本文要讲的重点。

为了照顾一些读者大人的耐心，这里直接说结论。

**可以共享的资源：** 

*   纹理；
*   shader；
*   program 着色器程序；
*   buffer 类对象，如 VBO、 EBO、 RBO 等 。

**不可以共享的资源：** 

*   FBO 帧缓冲区对象（不属于 buffer 类）；
*   VAO 顶点数组对象（不属于 buffer 类）。

**这里解释下，在不可以共享的资源中，FBO 和 VAO 属于资源管理型对象，FBO 负责管理几种缓冲区，本身不占用资源，VAO 负责管理 VBO 或 EBO ，本身也不占用资源。** 

结论说完了，将在下一节进行结论验证，我们将在主渲染线程之外开辟一个新的渲染线程，然后将主渲染线程生成的纹理、 program 等资源分享给新的渲染线程使用。

![](https://ask.qcloudimg.com/http-save/7399394/wf182xpf51.png?imageView2/2/w/1620)

共享上下文多线程渲染

本小节**将在主渲染线程之外通过共享 EGLContext 的方式开辟一个新的离屏渲染线程，之后将主渲染线程生成的纹理、 program 、VBO 资源分享给新的渲染线程使用**，最后将保存（新渲染线程）渲染结果的纹理返回给主线程进行上屏渲染。

共享上下文
-----

在 EGL\_VERSION\_1\_4 （Android 5.0）版本，在当前渲染线程直接调用 eglGetCurrentContext 就可以直接获取到上下文对象 EGLContext 。

C++ ，Java 层均有对应获取上下文对象的 API 实现：

我们在新线程中使用 EGL 创建渲染环境时，通过主渲染线程获取的 `sharedContext` 来创建新线程的上下文对象。

由于我们在新线程要渲染到屏幕外的区域，需要创建 PbufferSurface 。

国际惯例，我们将 EGL 的操作都封装到一个类 EglCore 中方便使用，具体代码可以参考文末的项目。

多线程渲染
-----

类比 Android Java 层的 Looper 类，我们在 C++ 实现 Looper 用于创建新线程并管理线程中的消息。

在 GLRenderLooper 类中分别定义 OnSurfaceCreated、 OnSurfaceChanged、 OnDrawFrame 用于处理对应的事件。

在函数 GLRenderLooper::OnSurfaceCreated 中，利用 sharedContext 创建 OpenGL 渲染环境。

GLRenderLooper::OnDrawFrame 函数中，绘制完成注意交换缓冲区，然后将保存绘制结果的纹理，通过回调函数传递给主线程进行上屏渲染。

回到渲染主线程，Init 时将主渲染生成的纹理、 program 、VBO 资源以及 EGLContext 传递给新线程。

主线程渲染时，首先向新线程发送渲染指令，然后等待其渲染结束，新线程渲染结束后会调用 OnAsyncRenderDone 函数通知主线程进行上屏渲染。

最后需要注意的是：**多线程渲染要确保纹理等共享资源不会被同时访问，否则会导致渲染出错。** 

完整代码参考下面项目，选择 Multi-Thread Render：

原创声明，本文系作者授权云+社区发表，未经许可，不得转载。

如有侵权，请联系 yunjia\_community@tencent.com 删除。