#include "types.hpp"
#include "measures.hpp"
#include "iterative_methods.hpp"
#include "min_lag_method.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_opengles2.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <chrono>
#include <SDL2/SDL.h>
#include <time.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_thread.h>
#include <iomanip>
#include <dlfcn.h>
#include <glm/glm.hpp>
#include <sstream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <thread>
#include <string>
#include <iostream>
#include <cstring>
#include <X11/Xlib.h>
#include <atomic>
#include <string>
#include <mutex>

const int WINDOW_WIDTH = 800;  // Ширина окна

const int WINDOW_HEIGHT = 800; // Высота окна
const float SCALE_Y = 30.0f;
std::atomic<bool> draw(false);
std::atomic<bool> shader(false);
std::atomic<bool> stop_thread(false);
std::atomic<bool> stop_prog(false);
std::atomic<int> cnt(0);
std::mutex thread_mutex;

typedef int (*run2d_func)(fp *, fp *, fp *, const fp *, const fp *, uint32_t, uint32_t);
typedef void (*run1d_func)(fp *, const fp *, const fp *, uint32_t);



// Закрытие SDL
void cleanupSDL(SDL_Window *window, SDL_Renderer *renderer) {
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}
// Функция для компиляции шейдера
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "ERROR: Shader compilation failed\n" << infoLog << std::endl;
        return 0;
    }
    return shader;
}

// Функция для создания и линковки шейдерной программы
GLuint createShaderProgram() {
    const char* vertexShaderSource = R"(
        #version 300 es
        uniform mat4 uModelViewProjection;
        in vec3 aPosition;
        in vec3 aColor;
        out vec3 vColor;
        void main() {
            gl_Position = uModelViewProjection * vec4(aPosition, 1.0);
            vColor = aColor;
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 300 es
        precision mediump float;
        in vec3 vColor;
        out vec4 fragColor;
        void main() {
            fragColor = vec4(vColor, 1.0);
        }
    )";

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
	printf("LINK ERROR\n");
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

// Глобальные переменные, которые будут обновляться в другой функции
double fps_opt = 60.0f;
double fps_ref = 0.0;
double gflops_opt = 120.5f;
double gflops_ref = 0.0;
double lps_opt = 500.0f;
double lps_ref = 0.0;
double time_opt = 0.0;
double time_ref = 0.0;

// Проверка, находится ли точка внутри прямоугольника
bool isPointInRect(int x, int y, SDL_Rect rect) {
    return (x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h);
}

int testSDLWindow(void* data) {

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Создание окна
    SDL_Window* window = SDL_CreateWindow("Main window", 0, 650, 1600, 400, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        TTF_Quit();
        SDL_Quit();
        return 0;
    }

    // Создание рендерера
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Загрузка шрифта
    TTF_Font* font = TTF_OpenFont("Arial_Cyr.ttf", 90);
    if (!font) {
        std::cerr << "Failed to load font! TTF_Error: " << TTF_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }

    // Цвет текста
    SDL_Color textColor = {255, 255, 255, 255};  // Белый цвет

    // Создаем текст для заголовка
    SDL_Surface* titleSurface = TTF_RenderText_Solid(font, "Membrane oscillation problem", textColor);
    if (!titleSurface) {
        std::cerr << "Failed to create title surface! TTF_Error: " << TTF_GetError() << std::endl;
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }
    SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
    SDL_FreeSurface(titleSurface);

    // Вычисляем позицию текста для заголовка
    SDL_Rect titleRect = {0, 10, float(titleSurface->w)*0.85*0.6, titleSurface->h*0.6};
    titleRect.x = (1600 - titleRect.w) / 2;  // Центрируем текст по горизонтали

    // Создаем текст для кнопки
    SDL_Surface* buttonTextSurface = TTF_RenderText_Solid(font, "Restart", textColor);
    if (!buttonTextSurface) {
        std::cerr << "Failed to create button text surface! TTF_Error: " << TTF_GetError() << std::endl;
        SDL_DestroyTexture(titleTexture);
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }
    SDL_Texture* buttonTextTexture = SDL_CreateTextureFromSurface(renderer, buttonTextSurface);
    SDL_FreeSurface(buttonTextSurface);

    // Определяем размеры кнопки и текст на кнопке
    SDL_Rect buttonRect = {700, 150, 200, 100};  // x, y, width, height
    SDL_Rect buttonTextRect = {0, 0, buttonTextSurface->w*0.5, buttonTextSurface->h*0.5};
    buttonTextRect.x = buttonRect.x + (buttonRect.w - buttonTextRect.w) / 2;
    buttonTextRect.y = buttonRect.y + (buttonRect.h - buttonTextRect.h) / 2;

    // Функция для обновления текста с глобальными переменными
    auto createTextTexture = [&](const std::string& text) -> SDL_Texture* {
        SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), textColor);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        return texture;
    };

    // Главный цикл программы
    while (true) {
        // Очистка экрана
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);  // Черный фон
        SDL_RenderClear(renderer);

        // Рисуем заголовок
        SDL_RenderCopy(renderer, titleTexture, nullptr, &titleRect);
	

        // Рисуем кнопку
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);  // Красный цвет
        SDL_RenderFillRect(renderer, &buttonRect);

        // Рисуем текст на кнопке
        SDL_RenderCopy(renderer, buttonTextTexture, nullptr, &buttonTextRect);

        // Рисуем новые тексты: Reference BLAS, Optimized BLAS
        SDL_Texture* referenceTextTexture = createTextTexture("Optimized BLAS");
        SDL_Rect referenceRect = {100, titleRect.y + 20, 300, 40};  // слева от центрального текста
        SDL_RenderCopy(renderer, referenceTextTexture, nullptr, &referenceRect);
        SDL_DestroyTexture(referenceTextTexture);

        SDL_Texture* optimizedTextTexture = createTextTexture("Reference BLAS");
        SDL_Rect optimizedRect = {1200, titleRect.y + 20, 300, 40};  // справа от центрального текста
        SDL_RenderCopy(renderer, optimizedTextTexture, nullptr, &optimizedRect);
        SDL_DestroyTexture(optimizedTextTexture);

        // Рисуем значения FPS, GFLOPS, Performance boost, LPS
	std::stringstream nums;

	nums<<std::fixed<<std::setprecision(2)<<fps_opt;
        std::string fpsText = "FPS: " + nums.str();
	nums.str(std::string());

	nums<<std::fixed<<std::setprecision(2)<<gflops_opt;
        std::string gflopsText = "MFLOPS: " + nums.str();
	nums.str(std::string());

	nums<<std::fixed<<std::setprecision(2)<<lps_opt;
        std::string lpsText = "LPS: " + nums.str();
	nums.str(std::string());

	nums<<std::fixed<<std::setprecision(2)<<(gflops_opt/gflops_ref);
	std::string AccText = "Speedup: " + nums.str();
	nums.str(std::string());

	nums<<std::fixed<<std::setprecision(2)<<(time_opt);
	std::string tText = "t = " + nums.str();
	nums.str(std::string());


	SDL_Texture* tTextTexture = createTextTexture(tText);
        SDL_Rect tRect = {100, 270, 130, 40};  // под Reference BLAS
        SDL_RenderCopy(renderer, tTextTexture, nullptr, &tRect);
        SDL_DestroyTexture(tTextTexture);

	SDL_Texture* AccTextTexture = createTextTexture(AccText);
        SDL_Rect AccRect = {700, 300, 200, 40};  // под Reference BLAS
        SDL_RenderCopy(renderer, AccTextTexture, nullptr, &AccRect);
        SDL_DestroyTexture(AccTextTexture);



        SDL_Texture* fpsTextTexture = createTextTexture(fpsText);
        SDL_Rect fpsRect = {100, referenceRect.y + 60, 150, 40};  // под Reference BLAS
        SDL_RenderCopy(renderer, fpsTextTexture, nullptr, &fpsRect);
        SDL_DestroyTexture(fpsTextTexture);

        SDL_Texture* gflopsTextTexture = createTextTexture(gflopsText);
        SDL_Rect gflopsRect = {100, fpsRect.y + 60, 200, 40};
        SDL_RenderCopy(renderer, gflopsTextTexture, nullptr, &gflopsRect);
        SDL_DestroyTexture(gflopsTextTexture);

        SDL_Texture* lpsTextTexture = createTextTexture(lpsText);
        SDL_Rect lpsRect = {100, gflopsRect.y + 60, 150, 40};
        SDL_RenderCopy(renderer, lpsTextTexture, nullptr, &lpsRect);
        SDL_DestroyTexture(lpsTextTexture);


	nums<<std::fixed<<std::setprecision(2)<<fps_ref;
        std::string fpsText_ref = "FPS: " + nums.str();
	nums.str(std::string());

	nums<<std::fixed<<std::setprecision(2)<<gflops_ref;
        std::string gflopsText_ref = "MFLOPS: " + nums.str();
	nums.str(std::string());

	nums<<std::fixed<<std::setprecision(2)<<lps_ref;
        std::string lpsText_ref = "LPS: " + nums.str();
	nums.str(std::string());

	nums<<std::fixed<<std::setprecision(2)<<(time_ref);
	std::string tText_ref = "t = : " + nums.str();
	nums.str(std::string());


	SDL_Texture* tTextTexture_ref = createTextTexture(tText_ref);
        SDL_Rect tRect_ref = {1200, 270, 130, 40};  // под Reference BLAS
        SDL_RenderCopy(renderer, tTextTexture_ref, nullptr, &tRect_ref);
        SDL_DestroyTexture(tTextTexture_ref);

        SDL_Texture* fpsTextTexture_ref = createTextTexture(fpsText_ref);
        SDL_Rect fpsRect_ref = {1200, referenceRect.y + 60, 150, 40};  // под Reference BLAS
        SDL_RenderCopy(renderer, fpsTextTexture_ref, nullptr, &fpsRect_ref);
        SDL_DestroyTexture(fpsTextTexture_ref);

        SDL_Texture* gflopsTextTexture_ref = createTextTexture(gflopsText_ref);
        SDL_Rect gflopsRect_ref = {1200, fpsRect.y + 60, 200, 40};
        SDL_RenderCopy(renderer, gflopsTextTexture_ref, nullptr, &gflopsRect_ref);
        SDL_DestroyTexture(gflopsTextTexture_ref);

        SDL_Texture* lpsTextTexture_ref = createTextTexture(lpsText_ref);
        SDL_Rect lpsRect_ref = {1200, gflopsRect.y + 60, 150, 40};
        SDL_RenderCopy(renderer, lpsTextTexture_ref, nullptr, &lpsRect_ref);
        SDL_DestroyTexture(lpsTextTexture_ref);


        // Обновляем экран
        SDL_RenderPresent(renderer);

        // Обработка событий
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                stop_prog = true;
	        SDL_DestroyRenderer(renderer);
    		SDL_DestroyWindow(window);
                return 0;
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mouseX = event.button.x;
                int mouseY = event.button.y;
                // Проверяем, находится ли клик внутри кнопки
                if (isPointInRect(mouseX, mouseY, buttonRect)) {
                    stop_thread = true;
                    while (cnt==0) {int b = 3;}
                    while (cnt==1) 
                    {
                        int bb = 44;
                    }
                    stop_thread = false;
                    cnt=0;
                }
            }
        }
    }
    return 0;
}
// Функция для рисования 3D поверхности
void drawWaveSurface(const std::vector<double>& wave_data, const int n, const int k)
{
    std::vector<GLfloat> vertices;
    std::vector<GLfloat> colors;

    for (int i = 0; i < k - 1; ++i)
    {
        for (int j = 0; j < n - 1; ++j)
        {
            float r = 1.0f + wave_data[i * n + j]/1.9f;
            float g =1.0f - std::fabs(wave_data[i*n+j])/3.5;
            float b = 1.0f - wave_data[i * n + j]/1.9f;

            // Треугольник 1
            vertices.push_back(float(j) * 100.0f / n);           // X1
            vertices.push_back(float(i) * 100.0f / k);           // Y1
            vertices.push_back(wave_data[i * n + j] * 3);        // Z1

            vertices.push_back(float(j + 1) * 100.0f / n);       // X2
            vertices.push_back(float(i) * 100.0f / k);           // Y2
            vertices.push_back(wave_data[i * n + j + 1] * 3);    // Z2

            vertices.push_back(float(j) * 100.0f / n);           // X3
            vertices.push_back(float(i + 1) * 100.0f / k);       // Y3
            vertices.push_back(wave_data[(i + 1) * n + j] * 3);  // Z3

            // Цвета для треугольника 1
            for (int v = 0; v < 3; ++v) {
                colors.push_back(r);
                colors.push_back(g);
                colors.push_back(b);
            }

            // Треугольник 2
            vertices.push_back(float(j + 1) * 100.0f / n);       // X1
            vertices.push_back(float(i) * 100.0f / k);           // Y1
            vertices.push_back(wave_data[i * n + j + 1] * 3);    // Z1

            vertices.push_back(float(j + 1) * 100.0f / n);       // X2
            vertices.push_back(float(i + 1) * 100.0f / k);       // Y2
            vertices.push_back(wave_data[(i + 1) * n + j + 1] * 3); // Z2

            vertices.push_back(float(j) * 100.0f / n);           // X3
            vertices.push_back(float(i + 1) * 100.0f / k);       // Y3
            vertices.push_back(wave_data[(i + 1) * n + j] * 3);  // Z3

            // Цвета для треугольника 2
            for (int v = 0; v < 3; ++v) {
                colors.push_back(r);
                colors.push_back(g);
                colors.push_back(b);
            }
        }
    }

    

    GLuint VAO, VBO, colorVBO;
    glm::vec3 cameraPos(0.0f, 50.0f, 75.0f);
    glm::vec3 cameraTarget(50.0f, 50.0f, 5.0f);
    glm::vec3 cameraUp(0.0f, 0.0f, 1.0f);
    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);

    glm::mat4 projection = glm::perspective(glm::radians(90.0f), (float)800 / (float)600, 0.1f, 100.0f);

    glm::mat4 modelViewProjection = projection * view;

    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &colorVBO);

    glBindVertexArray(VAO);
    GLuint shaderProgram = createShaderProgram();

    glUseProgram(shaderProgram);

    glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GLfloat), colors.data(), GL_STATIC_DRAW);

    GLint colAttrib = glGetAttribLocation(shaderProgram , "aColor");
    glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_TRUE, 3 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(colAttrib);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

    GLint posAttrib = glGetAttribLocation(shaderProgram , "aPosition");
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(posAttrib);

    GLuint mvpLocation = glGetUniformLocation(shaderProgram , "uModelViewProjection");
    glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, glm::value_ptr(modelViewProjection));
    


    glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 3);
    glBindVertexArray(0);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &colorVBO);
    glDeleteVertexArrays(1, &VAO);
}

    void solve_wave2d_kernel(
    fp T, uint n, uint m, uint k, fp a_sq,
    FUNCTION_XYT_TYPE f,
    FUNCTION_XY_TYPE phi1, FUNCTION_XY_TYPE phi2,
    FUNCTION_YT_TYPE xi1, FUNCTION_YT_TYPE xi2,
    FUNCTION_XT_TYPE xi3, FUNCTION_XT_TYPE xi4,
    void *handle, const std::string &name, const uint32_t pos,  SDL_Window* window, SDL_GLContext glContext, SDL_Renderer* renderer, fp* fps, fp* gflops, fp* lps, fp* time)
{

    int frame_count = 0;
    int start_time = 0;
    int is_draw = 0;


     SDL_GL_MakeCurrent(window, glContext);
    // Set OpenGL ES version to 3.0
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    glEnable(GL_DEPTH_TEST); // Enable depth testing
    glClearColor(0.6f, 0.6f, 0.6f, 1.0f); // Set the background color (white)

    auto run2d = (run2d_func)dlsym(handle, "run2d");
    if (!run2d)
    {
        std::cerr << "Не удалось найти функцию run2d: " << dlerror() << std::endl;
        dlclose(handle);
        return;
    }


    matrix V = matrix(m + 1, vector((n - 1) * (k + 1)));
    fp hx = 1.0 / (fp)n;
    fp hy = 1.0 / (fp)k;
    fp ht = T / (fp)m;

    fp hx_sq_inv = 1 / (hx * hx);
    fp hy_sq_inv = 1 / (hy * hy);
    fp ht_sq_inv = 1 / (ht * ht);

    int iters = 0;
    fp *b = (fp *)(malloc(sizeof(fp) * (n - 1) * (k - 1)));
    fp *up = (fp *)(malloc(sizeof(fp) * (n - 1) * (k - 2)));
    fp *down = (fp *)(malloc(sizeof(fp) * (n - 1) * (k - 2)));

    fp mA = ht_sq_inv + a_sq * (hx_sq_inv + hy_sq_inv);
    fp left = -a_sq * (hx_sq_inv) * 0.5;
    fp right = left;
    fp top = -a_sq * (hy_sq_inv) * 0.5;
    fp bottom = top;
    for (int i = 0; i < (n - 1) * (k - 2); ++i)
    {
        up[i] = top;
        down[i] = bottom;
    }
    memset(b, 0, sizeof(fp) * (n - 1) * (k - 1));

    fp *a = (fp *)(malloc(sizeof(fp) * (n - 1) * (k - 1) * 3));
    memset(a, 0, sizeof(fp) * (n - 1) * (k - 1) * 3);
    int kl = 1;
    int ku = 1;
    int n_ = (n - 1) * (k - 1);
    int m_ = (n - 1) * (k - 1);
    int lda = 3;
    int ldm = n_;
    for (int i = 0; i < m_; i++)
    {
        int k_ = kl - i;
        for (int j = std::max(0, i - kl); j < std::min(int(n_), i + ku + 1); j++)
        {
            // a[(k_ + j) + i * lda] = A[j + i * ldm];
            if ((j) == i)
                a[(k_ + j) + i * lda] = mA;
            else if (j == i - 1 && i % (n - 1))
                a[(k_ + j) + i * lda] = left;
            else if (j == (i + 1) && i % (n - 1) != (n - 2))
                a[(k_ + j) + i * lda] = right;
        }
    }
    for (uint j = 0; j <= m; j++)
    {
        for (int i = 1; i < n; ++i)
        {
            V[j][i - 1] = xi3(i * hx, j * ht);
            V[j][k * (n - 1) + i - 1] = xi4(i * hx, j * ht);
        }
    }

    for (int j = 0; j <= k; ++j)
    {
        for (int i = 1; i < n; ++i)
        {
            V[0][j * (n - 1) + i - 1] = phi1(i * hx, j * hy);
        }
    }

    for (int i = 1; i < n; ++i)
    {
        for (int j = 1; j < k; ++j)
        {
            fp tmp = phi2(i * hx, j * hy) + ht * 0.5 *(a_sq * ((hx_sq_inv * (phi1((i - 1) * hx, j * hy) - 2.0 * phi1(i * hx, j * hy) + phi1((i + 1) * hx, j * hy)) + 
                                                                hy_sq_inv * (phi1(i * hx, (j - 1) * hy) - 2.0 * phi1(i * hx, j * hy) + phi1(i * hx, (j + 1) * hy))) + f(i * hx, j * hy, 0))); // отыскание 1-го слоя со вторым порядком погрешности по времени
            V[1][j * (n - 1) + i - 1] = ht * tmp + V[0][j * (n - 1) + i - 1];
        }
    };
    matrix V_start = V;
	

    bool changed = false;

    while (!stop_prog)
    {
	V = V_start;
	is_draw = 0;

        for (int j = 2; j <= m; ++j)
        {

	    if (stop_prog){
                break;
            }
            if (stop_thread && !changed){
                cnt++;
		V = V_start;
		j=1;
		is_draw = 0;
                changed = true;
                continue;
            }
	    while (cnt==1){
               int mm=3;
	    }
            changed = false;

            for (int t = 1; t < k; ++t)
            {

                b[(t - 1) * (n - 1)] = a_sq * 0.5 * (hx_sq_inv * (V[j - 2][t * (n - 1) + 1] - 2 * V[j - 2][t * (n - 1)] + xi1(t * hy, ht * (j - 2))) + hy_sq_inv * (V[j - 2][(t + 1) * (n - 1)] - 2 * V[j - 2][t * (n - 1)] + V[j - 2][(t - 1) * (n - 1)])) +
                                       ht_sq_inv * (2 * V[j - 1][t * (n - 1)] - V[j - 2][t * (n - 1)]) + f(hx, t * hy, (j - 1) * ht);
                for (int i = 1; i < n - 2; ++i)
                {
                    b[(t - 1) * (n - 1) + i] = a_sq * 0.5 * (hx_sq_inv * (V[j - 2][t * (n - 1) + i + 1] - 2 * V[j - 2][t * (n - 1) + i] + V[j - 2][t * (n - 1) + i - 1]) + hy_sq_inv * (V[j - 2][(t + 1) * (n - 1) + i] - 2 * V[j - 2][t * (n - 1) + i] + V[j - 2][(t - 1) * (n - 1) + i])) +
                                               ht_sq_inv * (2 * V[j - 1][t * (n - 1) + i] - V[j - 2][t * (n - 1) + i]) + f(hx * (i + 1), t * hy, (j - 1) * ht);
                }
                int i = n - 2;
                b[(t - 1) * (n - 1) + i] = a_sq * 0.5 * (hx_sq_inv * (xi2(t * hy, ht * (j - 2)) - 2 * V[j - 2][t * (n - 1) + i] + V[j - 2][t * (n - 1) + i - 1]) + hy_sq_inv * (V[j - 2][(t + 1) * (n - 1) + i] - 2 * V[j - 2][t * (n - 1) + i] + V[j - 2][(t - 1) * (n - 1) + i])) +
                                           ht_sq_inv * (2 * V[j - 1][t * (n - 1) + i] - V[j - 2][t * (n - 1) + i]) + f(hx * (i + 1), t * hy, (j - 1) * ht);
            }

            for (int i = 1; i < k; ++i)
            {
                b[(i - 1) * (n - 1)] -= xi1(i * hy, j * ht) * left;
                b[(i - 1) * (n - 1) + n - 2] -= xi2(i * hy, j * ht) * right;
            }

            for (int i = 1; i < n; ++i)
            {
                b[i - 1] -= xi3(i * hx, j * ht) * bottom;
                b[(k - 2) * (n - 1) + i - 1] -= xi4(i * hx, j * ht) * top;
            }

            iters+=run2d(&V[j][n - 1], up, down, b, a, (n - 1) * (k - 1), n - 1);

            uint32_t draw_start, draw_finish, draw_time;
            if (is_draw == 0) {
		frame_count++;
		draw_start = SDL_GetTicks();
		glClear(GL_COLOR_BUFFER_BIT);
		drawWaveSurface(V[j], n - 1, k + 1); // Рисуем поверхность волны
		SDL_GL_SwapWindow(window);
		draw_finish = SDL_GetTicks();
		draw_time = draw_finish - draw_start;
	    }
	    is_draw = (is_draw + 1)%20;
            
            uint32_t current_time = SDL_GetTicks();
            uint32_t elapsed_time = current_time - start_time;
            if (frame_count>=20) // If 1 second has passed
            {
		*time = ht * j;
		*fps = float(frame_count*1000)/elapsed_time;
		*lps = 20000.0 / (elapsed_time - draw_time);
		*gflops = float(iters)*((n-1) * (k-1) * 6 + 9 * (n-1) * (k-2))/((elapsed_time - draw_time)*1000.0f);
                start_time = current_time;
                frame_count = 0;
		iters = 0;
            }

        }

    }
    free(a);
    free(b);
    free(up);
    free(down);
    SDL_DestroyRenderer(renderer);
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
}

int solve_wave2d_thread_func(void* data) {

    // Распаковываем данные
    auto params = static_cast<std::tuple<fp, uint, uint, uint, fp, FUNCTION_XYT_TYPE,
                                         FUNCTION_XY_TYPE, FUNCTION_XY_TYPE,
                                         FUNCTION_YT_TYPE, FUNCTION_YT_TYPE,
                                         FUNCTION_XT_TYPE, FUNCTION_XT_TYPE,
                                         void*, std::string, uint32_t, SDL_Window*, SDL_GLContext, SDL_Renderer*, fp*, fp*, fp*, fp*>*>(data);

    // Передаем параметры в solve_wave2d_kernel
    solve_wave2d_kernel(
        std::get<0>(*params), std::get<1>(*params), std::get<2>(*params),
        std::get<3>(*params), std::get<4>(*params), std::get<5>(*params),
        std::get<6>(*params), std::get<7>(*params), std::get<8>(*params),
        std::get<9>(*params), std::get<10>(*params), std::get<11>(*params),
        std::get<12>(*params), std::get<13>(*params), std::get<14>(*params), std::get<15>(*params), std::get<16>(*params), std::get<17>(*params),
	std::get<18>(*params), std::get<19>(*params), std::get<20>(*params), std::get<21>(*params) 
    );

    return 0;
}


void run_kernel(
    matrix& V,
    fp T,
    uint n, uint m, uint k, fp a_sq,
    FUNCTION_XYT_TYPE f,
    FUNCTION_XY_TYPE phi1, FUNCTION_XY_TYPE phi2,
    FUNCTION_YT_TYPE xi1, FUNCTION_YT_TYPE xi2,
    FUNCTION_XT_TYPE xi3, FUNCTION_XT_TYPE xi4
) {

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL2 Initialization failed: " << SDL_GetError() << std::endl;
        return;
    }

    if (TTF_Init() == -1) {
    	std::cerr << "SDL_ttf could not initialize! TTF_Error: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return;
    }

    // Create the SDL window with OpenGL ES 3.0 context
    SDL_Window* window1 = SDL_CreateWindow("Optimized BLAS", 0, 0, 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window1) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return;
    }

    SDL_GLContext glContext1 = SDL_GL_CreateContext(window1);
    if (!glContext1) {
        std::cerr << "OpenGL ES 3.0 context creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window1);
        SDL_Quit();
        return;
    }
    glEnable(GL_TEXTURE_2D);
    SDL_Renderer* renderer1 = SDL_CreateRenderer(window1, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer1) {
        std::cerr << "Ошибка создания рендерера SDL: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window1);
        SDL_Quit();
    }

    SDL_Window* window2 = SDL_CreateWindow("Reference BLAS", 810, 0, 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window2) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return;
    }

    SDL_GLContext glContext2 = SDL_GL_CreateContext(window2);
    if (!glContext2) {
        std::cerr << "OpenGL ES 3.0 context creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window2);
        SDL_Quit();
        return;
    }
    SDL_Renderer* renderer2 = SDL_CreateRenderer(window2, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer2) {
        std::cerr << "Ошибка создания рендерера SDL: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window2);
        SDL_Quit();
    }
    // Загружаем библиотеки
    void* handle1 = dlopen("libsolver_openblas1.so", RTLD_NOW);
    if (!handle1) {
        std::cerr << "Ошибка при загрузке библиотеки: " << dlerror() << std::endl;
        return;
    }

    void* handle2 = dlopen("libsolver_openblas2.so", RTLD_NOW);
    if (!handle2) {
        std::cerr << "Ошибка при загрузке библиотеки: " << dlerror() << std::endl;
        return;
    }

    // Подготовка данных для потоков
    auto params1 = new std::tuple<fp, uint, uint, uint, fp, FUNCTION_XYT_TYPE,
                                  FUNCTION_XY_TYPE, FUNCTION_XY_TYPE,
                                  FUNCTION_YT_TYPE, FUNCTION_YT_TYPE,
                                  FUNCTION_XT_TYPE, FUNCTION_XT_TYPE,
                                  void*, std::string, uint32_t, SDL_Window*, SDL_GLContext, SDL_Renderer*, fp*, fp*, fp*, fp*>(
        T, n, m, k, a_sq, f, phi1, phi2, xi1, xi2, xi3, xi4,
        handle1, "Optimized BLAS", 0, window1, glContext1, renderer1, &fps_opt, &gflops_opt, &lps_opt, &time_opt
    );

    auto params2 = new std::tuple<fp, uint, uint, uint, fp, FUNCTION_XYT_TYPE,
                                  FUNCTION_XY_TYPE, FUNCTION_XY_TYPE,
                                  FUNCTION_YT_TYPE, FUNCTION_YT_TYPE,
                                  FUNCTION_XT_TYPE, FUNCTION_XT_TYPE,
                                  void*, std::string, uint32_t, SDL_Window*, SDL_GLContext, SDL_Renderer*, fp*, fp*, fp*, fp*>(
        T, n, m, k, a_sq, f, phi1, phi2, xi1, xi2, xi3, xi4,
        handle2, "Reference BLAS", 0, window2, glContext2, renderer2, &fps_ref, &gflops_ref, &lps_ref, &time_ref
    );

    //solve_wave2d_thread_func( params1);



    // Создаем SDL потоки
    void* data1;
    SDL_Thread* thread0 = SDL_CreateThread(testSDLWindow, "SolverThread", data1);
    SDL_Thread* thread1 = SDL_CreateThread(solve_wave2d_thread_func, "SolverThread1", params2);
    solve_wave2d_kernel(T,n, m, k, a_sq,f, phi1, phi2, xi1, xi2, xi3, xi4, handle1, "Optimized BLAS", 2,  window1, glContext1, renderer1, &fps_opt, &gflops_opt, &lps_opt, &time_opt);
  
    // Ожидаем завершения потоков 
    SDL_WaitThread(thread0, nullptr);
    SDL_WaitThread(thread1, nullptr);
    //SDL_WaitThread(thread2, nullptr);
    // Освобождаем ресурсы*/
    dlclose(handle1);
    dlclose(handle2);
    delete params1;
    delete params2;
    SDL_Quit();
}