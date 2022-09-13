#define GLEW_STATIC
#include <GL/glew.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_opengl.h>

#define _WINDOW_WIDTH  800
#define _WINDOW_HEIGHT 800

float modelview[16];
float projection[16];

char *fmap(const char *file_path);

void identity(float mat4[16]);
void ortho(float mat4[16], float t, float r, float b, float l, float n, float f);
void scale(float mat4[16], float w, float h, float d);
void translate(float mat4[16], float dx, float dy, float dz);

int get_random(int prev);

int main(int argc, char **argv)
{
        SDL_Init(SDL_INIT_EVERYTHING);
        IMG_Init(IMG_INIT_PNG);

        int WINDOW_WIDTH = _WINDOW_WIDTH;
        int WINDOW_HEIGHT = _WINDOW_HEIGHT;

        SDL_Window *window = SDL_CreateWindow("Bounce",
                                              SDL_WINDOWPOS_UNDEFINED,
                                              SDL_WINDOWPOS_UNDEFINED,
                                              WINDOW_WIDTH, WINDOW_HEIGHT,
                                              SDL_WINDOW_OPENGL);

        SDL_GLContext *context = SDL_GL_CreateContext(window);

        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        SDL_GetWindowSize(window, &WINDOW_WIDTH, &WINDOW_HEIGHT);

        if (GLEW_OK != glewInit()) {
                fprintf(stderr, "GLEW failed to initialise!\n");
                exit(EXIT_FAILURE);
        }

        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glViewport(0., 0., WINDOW_WIDTH, WINDOW_HEIGHT);
        glClearColor(0., 0., 0., 1.);

        GLint bounce_vao, bounce_vbo, bounce_ibo;
        glGenVertexArrays(1, &bounce_vao);
        glBindVertexArray(bounce_vao);

        glGenBuffers(1, &bounce_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, bounce_vbo);

        glGenBuffers(1, &bounce_ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bounce_ibo);

        GLfloat vertices[] = {
                0., 0., 0., 0.,
                0., 1., 0., 1.,
                1., 1., 1., 1.,
                1., 0., 1., 0.,
        };

        GLuint indices[] = {
                0, 1, 2,
                2, 3, 0,
        };

        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        GLuint vertex_shader, fragment_shader, shader_program;
        float x, y, dx, dy, w, h;
        unsigned int u_random = 0;

        x = 0.;
        y = 0.;

        dx = 1.;
        dy = 1.;

        w = 200.;
        h = 200.;

        char *vertex_src = fmap("bounce.glvs");
        vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex_shader, 1, (const GLchar **)&vertex_src, NULL);
        glCompileShader(vertex_shader);

        GLint success = GL_FALSE;
        glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
        if (success != GL_TRUE) {
                char msg[1024];
                glGetShaderInfoLog(vertex_shader, 1024, NULL, msg);
                printf("%s\n", msg);
        }
        free(vertex_src);

        char *fragment_src = fmap("bounce.glfs");
        fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment_shader, 1, (const GLchar **)&fragment_src, NULL);
        glCompileShader(fragment_shader);

        success = GL_FALSE;
        glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
        if (success != GL_TRUE) {
                char msg[1024];
                glGetShaderInfoLog(fragment_shader, 1024, NULL, msg);
                printf("%s\n", msg);
        }
        free(fragment_src);

        shader_program = glCreateProgram();
        glAttachShader(shader_program, vertex_shader);
        glAttachShader(shader_program, fragment_shader);
        glLinkProgram(shader_program);
        glUseProgram(shader_program);

        GLint vertex_attrib = glGetAttribLocation(shader_program, "vertex");
        glEnableVertexAttribArray(vertex_attrib);
        glVertexAttribPointer(vertex_attrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GL_FLOAT), 0);

        GLint texture_attrib = glGetAttribLocation(shader_program, "tex_vertex_in");
        glEnableVertexAttribArray(texture_attrib);
        glVertexAttribPointer(texture_attrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GL_FLOAT),
                              (void *)(2 * sizeof(GL_FLOAT)));

        identity(modelview);
        ortho(projection, 0., WINDOW_WIDTH, WINDOW_HEIGHT, 0., 10., -10.);

        GLuint texture;

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        SDL_Surface *surface = IMG_Load("dvd-logo.png");

        if (surface == NULL) {
                fprintf(stderr , "Failed to load file!\n");
                exit(EXIT_FAILURE);
        }

        w = surface->w;
        h = surface->h;

        glTexImage2D(GL_TEXTURE_2D, 0, surface->format->BytesPerPixel, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        SDL_FreeSurface(surface);

        glUniform1i(glGetUniformLocation(shader_program, "random"), u_random);

        int running = 1;
        SDL_Event event = {0};
        while (running) {
                while (SDL_PollEvent(&event)) {
                        if (event.type == SDL_QUIT || event.key.keysym.sym == SDLK_ESCAPE)
                                running = 0;
                }
                glClear(GL_COLOR_BUFFER_BIT);

                if (x <= 0.) {
                        u_random = get_random(u_random);
                        glUniform1i(glGetUniformLocation(shader_program, "random"), u_random);
                        dx = 1.;
                } else if (x >= WINDOW_WIDTH-w) {
                        u_random = get_random(u_random);
                        glUniform1i(glGetUniformLocation(shader_program, "random"), u_random);
                        dx = -1.;
                } if (y <= 0.) {
                        u_random = get_random(u_random);
                        glUniform1i(glGetUniformLocation(shader_program, "random"), u_random);
                        dy = 1.;
                } else if (y >= WINDOW_HEIGHT-h) {
                        u_random = get_random(u_random);
                        glUniform1i(glGetUniformLocation(shader_program, "random"), u_random);
                        dy = -1.;
                }

                x += dx;
                y += dy;

                translate(modelview, x, y, 0.);
                scale(modelview, w, h, 1.);
                glBufferSubData(GL_ARRAY_BUFFER, 0, 6 * sizeof(*vertices), vertices);
                glUniformMatrix4fv(glGetUniformLocation(shader_program, "modelview"), 1, GL_FALSE,  modelview);
                glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection"), 1, GL_FALSE, projection);

                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                SDL_GL_SwapWindow(window);
        }

        SDL_DestroyWindow(window);
        SDL_GL_DeleteContext(context);
        SDL_Quit();
        return 0;
}

char *fmap(const char *file_path)
{
        FILE *fp;
        size_t file_size;
        char *file_buffer;
        int i, c;

        fp = fopen(file_path, "r");

        if (fp == NULL) {
                fprintf(stderr, "could not open file '%s'\n", file_path);
                return NULL;
        }

        fseek(fp, 0, SEEK_END);
        file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        file_buffer = malloc(sizeof(*file_buffer) * (file_size + 1));

        if (file_buffer == NULL) {
                fprintf(stderr, "could not allocate memory for buffer!\n");
                return NULL;
        }

        for (i = 0; (c = fgetc(fp)) != EOF; ++i)
                file_buffer[i] = c;
        file_buffer[i] = '\0';

        return file_buffer;
}


void identity(float mat4[16])
{
        int i;
        for (i = 0; i < 16; ++i)
                mat4[i] = 0.;
        mat4[0]  = 1.;
        mat4[5]  = 1.;
        mat4[10] = 1.;
        mat4[15] = 1.;
}

void ortho(float mat4[16], float t, float r, float b, float l, float n, float f)
{
        int i;
        for (i = 0; i < 16; ++i)
                mat4[i] = 0.;
        mat4[0]  = 2/(r-l);
        mat4[5]  = 2/(t-b);
        mat4[10] = -2/(f-n);
        mat4[12] = -(r+l)/(r-l);
        mat4[13] = -(t+b)/(t-b);
        mat4[14] = -(f+n)/(f-n);
        mat4[15] = 1.;
}

void scale(float mat4[16], float w, float h, float d)
{
        mat4[0]  = w;
        mat4[5]  = h;
        mat4[10] = d;
}

void translate(float mat4[16], float dx, float dy, float dz)
{
        mat4[12] = dx;
        mat4[13] = dy;
        mat4[14] = dz;
}

int get_random(int prev)
{
        int random = rand() % 7;
        if (random == prev) {
                if (random < 6)
                        ++random;
                else
                        --random;
        }

        return random;
}
