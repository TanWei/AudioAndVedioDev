项目地址https://learnopengl-cn.github.io/  
glfw：创建窗口，管理上下文context  
glad：管理opengl指针  

顶点缓冲对象：Vertex Buffer Object，VBO
![图片](./顶点缓冲对象.png)
顶点数组对象：Vertex Array Object，VAO
![图片](./顶点数组对象.png)
索引缓冲对象：Element Buffer Object，EBO或Index Buffer Object，IBO

3D坐标转为2D坐标的处理过程是由OpenGL的图形渲染管线  

当今大多数显卡都有成千上万的小处理核心，它们在GPU上为每一个（渲染管线）阶段运行各自的小程序，从而在图形渲染管线中快速处理你的数据。这些小程序叫做着色器(Shader)。

简单起见，我们还是假定每个顶点只由一个3D位置(译注1)和一些颜色值组成
![图片](./%E6%B8%B2%E6%9F%93%E7%AE%A1%E7%BA%BF%E6%8A%BD%E8%B1%A1%E6%AD%A5%E9%AA%A4.png)

gpu和显存类似于，cpu和内存的关系
```
// 定点着色器
#version 330 core
layout (location = 0) in vec3 aPos;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}

// 片段着色器
#version 330 core
out vec4 FragColor;

void main()
{
    // 全橙
    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
} 

```
```
//************************************
//*          VBO使用方法             *
//************************************
// 0. 复制顶点数组到缓冲中供OpenGL使用
glBindBuffer(GL_ARRAY_BUFFER, VBO);
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
// 1. 设置顶点属性指针
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
glEnableVertexAttribArray(0);
// 2. 当我们渲染一个物体时要使用着色器程序
glUseProgram(shaderProgram);
// 3. 绘制物体
someOpenGLFunctionThatDrawsOurTriangle();
//************************************
//*          VAO使用方法             *
//************************************
// ..:: 初始化代码（只运行一次 (除非你的物体频繁改变)） :: ..
// 1. 绑定VAO
glBindVertexArray(VAO);
// 2. 把顶点数组复制到缓冲中供OpenGL使用
glBindBuffer(GL_ARRAY_BUFFER, VBO);
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
// 3. 设置顶点属性指针
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
glEnableVertexAttribArray(0);

[...]

// ..:: 绘制代码（渲染循环中） :: ..
// 4. 绘制物体
glUseProgram(shaderProgram);
glBindVertexArray(VAO);
someOpenGLFunctionThatDrawsOurTriangle();
```