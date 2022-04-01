GLuint prog_yuv;
GLuint texUniformY,texUniformU,texUniformV;
GLuint tex_yuv[3];
enum E_VER_ATTR{ver_attr_ver = 3, ver_attr_tex = 4, ver_attr_num};

struct Texture{
    GLuint texID; // glGenTextures分配的ID
    GLuint type; // 数据类型如GL_RGB
    GLint width; 
    GLint height;
    GLint bpp;
    GLubyte* data; // 像素数据
};

// 加载YUV着色器
void loadYUVShader(){
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);

    char szVS[] = "             \
    in vec4 verIn;       \
    in vec2 texIn;       \
    out vec2 texOut;        \
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
    out vec2 texOut;        \
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

//#ifdef _DEBUG
    GLint iRet = 0;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &iRet);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &iRet);
//#endif

    prog_yuv = glCreateProgram();

    glAttachShader(prog_yuv, vs);
    glAttachShader(prog_yuv, fs);

    glBindAttribLocation(prog_yuv, ver_attr_ver, "verIn");
    glBindAttribLocation(prog_yuv, ver_attr_tex, "texIn");

    glLinkProgram(prog_yuv);

//#ifdef _DEBUG
    glGetProgramiv(prog_yuv, GL_LINK_STATUS, &iRet);
//#endif

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

    // reverse
    //static const GLfloat textures[] = {
    //  0.0f, 0.0f,
    //  1.0f, 0.0f,
    //  0.0f, 1.0f,
    //  1.0f, 1.0f,
    //};

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

// 绘画YUV数据
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