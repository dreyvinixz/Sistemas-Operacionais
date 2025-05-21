#include <opencv2/opencv.hpp> // Biblioteca do OpenCV para manipulação de imagens
#include <iostream>
#include <pthread.h>

// Estrutura com dados para cada thread
struct ThreadInfo
{
    cv::Mat *inputImage;      // Ponteiro para imagem original (colorida)
    cv::Mat *outputGrayImage; // Ponteiro para imagem de saída (em tons de cinza)
    int startRow;             // Linha inicial a ser processada pela thread
    int endRow;               // Linha final a ser processada
    int threadIndex;          // Índice da thread
};

// Função executada por cada thread para converter parte da imagem para escala de cinza
void *convertSectionToGray(void *arg)
{
    ThreadInfo *info = (ThreadInfo *)arg;
    int imageWidth = info->inputImage->cols;

    std::cout << "Thread " << info->threadIndex
              << " iniciando: linhas " << info->startRow
              << " até " << info->endRow - 1 << "\n";

    // Processa as linhas atribuídas à thread
    for (int row = info->startRow; row < info->endRow; ++row)
    {
        cv::Vec3b *colorPixels = info->inputImage->ptr<cv::Vec3b>(row); // Linha colorida
        uchar *grayPixels = info->outputGrayImage->ptr<uchar>(row);     // Linha cinza

        for (int col = 0; col < imageWidth; ++col)
        {
            cv::Vec3b bgr = colorPixels[col];
            // Conversão RGB para escala de cinza (padrão luminosidade)
            grayPixels[col] = static_cast<uchar>(0.11 * bgr[0] + 0.59 * bgr[1] + 0.30 * bgr[2]);
        }
    }

    std::cout << "Thread " << info->threadIndex << " terminou.\n";
    pthread_exit(nullptr); // Finaliza a thread
}

int main()
{
    // Carrega imagem de entrada
    cv::Mat inputImage = cv::imread("exemplo.jpg");
    if (inputImage.empty())
    {
        std::cerr << "Erro ao carregar a imagem.\n";
        return 1;
    }

    // Cria imagem de saída em escala de cinza
    cv::Mat outputGrayImage(inputImage.rows, inputImage.cols, CV_8UC1);

    const int THREAD_COUNT = 4;          // Especifica o número de threads
    pthread_t threads[THREAD_COUNT];     // Identificadores das threads
    ThreadInfo threadData[THREAD_COUNT]; // Dados para cada thread

    // Divide as linhas da imagem entre as threads
    int totalRows = inputImage.rows;
    int baseRows = totalRows / THREAD_COUNT;
    int remainder = totalRows % THREAD_COUNT;

    for (int t = 0; t < THREAD_COUNT; ++t)
    {
        int start = 0;
        int rowsForThread = baseRows;
        // Distribui o restante entre as primeiras threads
        if (t < remainder)
        {
            rowsForThread += 1;
        }

        // Calcula linha inicial para a thread atual
        if (t < remainder)
        {
            start = t * (baseRows + 1);
        }
        else
        {
            start = t * baseRows + remainder;
        }

        // Preenche os dados da thread
        threadData[t].inputImage = &inputImage;
        threadData[t].outputGrayImage = &outputGrayImage;
        threadData[t].startRow = start;
        threadData[t].endRow = start + rowsForThread;
        threadData[t].threadIndex = t;

        // Cria a thread
        pthread_create(&threads[t], nullptr, convertSectionToGray, &threadData[t]);
        std::cout << "Thread " << t
                  << " -> linhas [" << threadData[t].startRow
                  << ", " << threadData[t].endRow - 1 << "]\n";
    }

    // Aguarda todas as threads finalizarem
    for (int t = 0; t < THREAD_COUNT; ++t)
    {
        pthread_join(threads[t], nullptr);
    }

    // Salva imagem convertida
    cv::imwrite("saida_gray.jpg", outputGrayImage);

    // Exibe imagens (colorida e em tons de cinza)
    cv::namedWindow("Original Color", cv::WINDOW_NORMAL);
    cv::namedWindow("Gray Scale", cv::WINDOW_NORMAL);
    cv::resizeWindow("Original Color", 800, 600);
    cv::resizeWindow("Gray Scale", 800, 600);
    cv::imshow("Original Color", inputImage);
    cv::imshow("Gray Scale", outputGrayImage);
    cv::waitKey(0); // Espera tecla para fechar

    return 0;
}
