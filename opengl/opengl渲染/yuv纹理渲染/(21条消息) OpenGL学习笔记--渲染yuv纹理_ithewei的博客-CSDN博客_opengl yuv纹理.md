一般[ffmpeg](https://so.csdn.net/so/search?q=ffmpeg&spm=1001.2101.3001.7020)解码后的数据类型都是I420，即YUV420P，OpenGL没有提供直接渲染yuv的接口，我们可以通过可编程渲染管线，利用多重纹理将Y、U、V纹理分别传入，在片元着色器GL\_FRAGMENT\_SHADER中将yuv进行矩阵转化成RGB，然后进行渲染。

GLSL简单介绍
--------

[OpenGL](https://so.csdn.net/so/search?q=OpenGL&spm=1001.2101.3001.7020)渲染管线的知识可以参考博客[http://www.cnblogs.com/yyxt/p/4056417.html](http://www.cnblogs.com/yyxt/p/4056417.html)  
顶点[着色器](https://so.csdn.net/so/search?q=%E7%9D%80%E8%89%B2%E5%99%A8&spm=1001.2101.3001.7020)和片元着色器是必须的。

GLSL的语法和C语言很类似。每一个[Shader](https://so.csdn.net/so/search?q=Shader&spm=1001.2101.3001.7020)程序都有一个main函数，这一点和c语言是一样的。这里的变量命名规则保持跟c一样就行了，注意gl\_开头的变量名是系统内置的变量。  
变量类型：  
attribute：外部传入顶点着色器的变量，每一个顶点都会有这两个属性。变化率高，用于定义每个点。  
varying：用于顶点着色器和片元着色器之间相互传递的参数。  
uniform：外部传入片元着色器的变量，变化率较低，对于可能在整个渲染过程没有改变，只是个常量。  
数据类型：  
vec2：包含了2个浮点数的向量  
vec3：包含了3个浮点数的向量  
vec4：包含了4个浮点数的向量  
sampler1D：1D[纹理](https://so.csdn.net/so/search?q=%E7%BA%B9%E7%90%86&spm=1001.2101.3001.7020)着色器  
sampler2D：2D纹理着色器  
sampler3D：3D纹理着色器  
mat2：2\*2维矩阵  
mat3：3\*3维矩阵  
mat4：4\*4维矩阵  
全局变量：  
gl\_Position：原始的顶点数据在Vertex Shader中经过平移、旋转、缩放等数学变换后，生成新的顶点位置（一个四维 (vec4) 变量，包含顶点的 x、y、z 和 w 值）。新的顶点位置通过在Vertex Shader中写入gl\_Position传递到[渲染管线](https://so.csdn.net/so/search?q=%E6%B8%B2%E6%9F%93%E7%AE%A1%E7%BA%BF&spm=1001.2101.3001.7020)的后继阶段继续处理。  
gl\_FragColor：[Fragment](https://so.csdn.net/so/search?q=Fragment&spm=1001.2101.3001.7020) Shader的输出，它是一个四维变量（或称为 vec4）。gl\_FragColor 表示在经过着色器代码处理后，正在呈现的像素的 R、G、B、A 值。  
Vertex Shader是作用于每一个顶点的，如果Vertex有三个点，那么Vertex Shader会被执行三次。Fragment Shader是作用于每个像素的，一个像素运行一次。从源代码中可以看出，像素的转换在Fragment Shader中完成。

**总结一句话就是顶点着色器搞定位置，片元着色器搞定颜色。** 

创建YUV着色器
--------

### 流程图：

![图片](./yuv%E7%BA%B9%E7%90%86%E6%B8%B2%E6%9F%93%E6%B5%81%E7%A8%8B%E5%9B%BE.png)

### 代码示例

```c++
GLuint prog_yuv;
GLuint texUniformY,texUniformU,texUniformV;
GLuint tex_yuv[3];
enum E_VER_ATTR{ver_attr_ver = 3, ver_attr_tex = 4, ver_attr_num};

struct Texture{
    GLuint texID; 
    GLuint type; 
    GLint width; 
    GLint height;
    GLint bpp;
    GLubyte* data; 
};


void loadYUVShader(){
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);

    char szVS[] = "             \
    attribute vec4 verIn;       \
    attribute vec2 texIn;       \
    varying vec2 texOut;        \
                                \
    void main(){                \
        gl_Position = verIn;    \
        texOut = texIn;         \
    }                           \
    ";
    const GLchar* pszVS = szVS;
    GLint len = strlen(szVS);
    glShaderSource(vs, 1, (const GLchar**)&pszVS, &len);

    char szFS[] = "             \
    varying vec2 texOut;        \
    uniform sampler2D tex_y;    \
    uniform sampler2D tex_u;    \
    uniform sampler2D tex_v;    \
                                \
    void main(){                \
        vec3 yuv;               \
        vec3 rgb;               \
        yuv.x = texture2D(tex_y, texOut).r;         \
        yuv.y = texture2D(tex_u, texOut).r - 0.5;   \
        yuv.z = texture2D(tex_v, texOut).r - 0.5;   \
        rgb = mat3( 1,       1,         1,          \
            0,       -0.39465,  2.03211,            \
            1.13983, -0.58060,  0) * yuv;           \
        gl_FragColor = vec4(rgb, 1);                \
    }                                               \
    ";
    const GLchar* pszFS = szFS;
    len = strlen(szFS);
    glShaderSource(fs, 1, (const GLchar**)&pszFS, &len);

    glCompileShader(vs);
    glCompileShader(fs);


    GLint iRet = 0;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &iRet);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &iRet);


    prog_yuv = glCreateProgram();

    glAttachShader(prog_yuv, vs);
    glAttachShader(prog_yuv, fs);

    glBindAttribLocation(prog_yuv, ver_attr_ver, "verIn");
    glBindAttribLocation(prog_yuv, ver_attr_tex, "texIn");

    glLinkProgram(prog_yuv);


    glGetProgramiv(prog_yuv, GL_LINK_STATUS, &iRet);


    glValidateProgram(prog_yuv);

    texUniformY = glGetUniformLocation(prog_yuv, "tex_y");
    texUniformU = glGetUniformLocation(prog_yuv, "tex_u");
    texUniformV = glGetUniformLocation(prog_yuv, "tex_v");


    static const GLfloat vertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f,
    };

    static const GLfloat textures[] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
    };

    
    
    
    
    
    
    

    glVertexAttribPointer(ver_attr_ver, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(ver_attr_ver);

    glVertexAttribPointer(ver_attr_tex, 2, GL_FLOAT, GL_FALSE, 0, textures);
    glEnableVertexAttribArray(ver_attr_tex);

    glGenTextures(3, tex_yuv);
    for (int i = 0; i < 3; i++){
        glBindTexture(GL_TEXTURE_2D, tex_yuv[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
}


void drawYUV(Texture* tex){
    glUseProgram(prog_yuv);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    int w = tex->width;
    int h = tex->height;
    int y_size = w*h;
    GLubyte* y = tex->data;
    GLubyte* u = y + y_size;
    GLubyte* v = u + (y_size>>2);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_yuv[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, y);
    glUniform1i(texUniformY, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex_yuv[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w/2, h/2, 0, GL_RED, GL_UNSIGNED_BYTE, u);
    glUniform1i(texUniformU, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, tex_yuv[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w/2, h/2, 0, GL_RED, GL_UNSIGNED_BYTE, v);
    glUniform1i(texUniformV, 2);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
```

这里只是贴出了关键的加载YUV着色器，和绘画YUV数据关键的代码，完整的结合ffmpeg的例子可以参考  
[http://blog.csdn.net/leixiaohua1020/article/details/40379845](http://blog.csdn.net/leixiaohua1020/article/details/40379845)雷大神的ffmpeg笔记，可惜天妒英才啊。