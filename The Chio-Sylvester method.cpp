#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <iostream>
#include <math.h>

// Структура для передачи данных в поток
struct ThreadData {
    double** matrix;  // Указатель на исходную матрицу
    int n;            // Размер матрицы
    double pivot;     // Опорный элемент
    int row;          // Строка для обработки
    int col;          // Столбец для обработки
    double* result;   // Указатель для сохранения результата
};

// Функция вычисления определителя матрицы 2x2
double det2(double** m) {
    return (m[0][0] * m[1][1]) - (m[0][1] * m[1][0]);
}

//Функция для считывания матрицы
void scanMatrix(double** matrix, int orderMatrix) {
    for (int i = 0; i < orderMatrix; ++i) {
        for (int j = 0; j < orderMatrix; ++j) {
            scanf("%lf", &matrix[i][j]);
        }
    }
}

//Функция для вывода матрицы в косоль
void printMatrix(double** matrix, int orderMatrix) {
    for (int i = 0; i < orderMatrix; ++i) {
        for (int j = 0; j < orderMatrix; ++j) {
            printf("%lf ", matrix[i][j]);
        }
        printf("\n");
    }
}

// Функция потока для вычисления элемента сконденсированной матрицы
DWORD WINAPI computeElement(LPVOID lpParam) {
    ThreadData* data = (ThreadData*)lpParam; // Структура данных потока

    // Создание временной матрицы 2x2
    double** tempMatrix = (double**)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 2 * sizeof(double*)); // Временная матрица
    tempMatrix[0] = (double*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 2 * sizeof(double));
    tempMatrix[1] = (double*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 2 * sizeof(double));

    // Заполняем значениями
    tempMatrix[0][0] = data->matrix[0][0];
    tempMatrix[0][1] = data->matrix[0][data->col + 1];
    tempMatrix[1][0] = data->matrix[data->row + 1][0];
    tempMatrix[1][1] = data->matrix[data->row + 1][data->col + 1];

    // Вычисляем и сохраняем результат
    *(data->result) = det2(tempMatrix);

    // Освобождаем память
    HeapFree(GetProcessHeap(), 0, tempMatrix[0]);
    HeapFree(GetProcessHeap(), 0, tempMatrix[1]);
    HeapFree(GetProcessHeap(), 0, tempMatrix);

    return 0;
}

// Функция вычисления определителя матрицы методом Чио-Сильвестра
double chioDeterminant(double** matrix, int n) {
    if (n == 1) return matrix[0][0];
    if (n == 2) return det2(matrix);

    int sign = 1; // Множетель знака определителя
   
    // Поиск опорного элемента
    int pillarRow = 0, pillarCol = 0;
    BOOL found = FALSE;
    for (int i = 0; i < n && !found; ++i) {
        for (int j = 0; j < n && !found; ++j) {
            if (matrix[i][j] != 0) {
                pillarRow = i;
                pillarCol = j;
                found = TRUE;
            }
        }
    }
    if (!found) return 0;
    
    // Перестройка матрицы
    if (pillarRow != 0) {
        for (int j = 0; j < n; ++j) {
            std::swap(matrix[pillarRow][j], matrix[0][j]);
        }
        sign *= -1;
    }
    if (pillarCol != 0) {
        for (int i = 0; i < n; ++i)
        {
            std::swap(matrix[i][pillarCol], matrix[i][0]);
        }
        sign *= -1;
    }
    if (found) {
        printf("\nRebuilt matrix\n");
        printMatrix(matrix, n);
    }

    double pillar = matrix[0][0]; // Опорный элемент
    int newSize = n - 1; // Размерность новой (сконденсированной) матрицы

    // Выделяем память под новую матрицу
    double** newMatrix = (double**)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, newSize * sizeof(double*)); // Сконденсированная матрица
    for (int i = 0; i < newSize; ++i) {
        newMatrix[i] = (double*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, newSize * sizeof(double));
    }

    // Подготавливаем многопоточное вычисление
    HANDLE* threads = (HANDLE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, newSize * newSize * sizeof(HANDLE)); // Потоки вычисления
    ThreadData* threadData = (ThreadData*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, newSize * newSize * sizeof(ThreadData)); // Данные для потоков

    // Запуск потоков
    int threadCount = 0; // Счётчик потоков
    for (int i = 0; i < newSize; ++i) {
        for (int j = 0; j < newSize; ++j) {
            threadData[threadCount] = { matrix, n, pillar, i, j, &newMatrix[i][j] };
            threads[threadCount] = CreateThread(NULL, 0, computeElement, &threadData[threadCount], 0, NULL);
            ++threadCount;
        }
    }

    // Ожидаем завершения потоков
    WaitForMultipleObjects(threadCount, threads, TRUE, INFINITE);


    // Освобождаем ресурсы
    for (int i = 0; i < threadCount; ++i) CloseHandle(threads[i]);
    HeapFree(GetProcessHeap(), 0, threads);
    HeapFree(GetProcessHeap(), 0, threadData);

    //Выводим новую матрицу
    printf("\nnewMatrix\n");
    printMatrix(newMatrix, newSize);

    // Рекурсивный вызов
    double det = chioDeterminant(newMatrix, newSize) / pow(pillar, n-2) * sign;

    // Освобождаем память
    for (int i = 0; i < newSize; ++i)
    {
        HeapFree(GetProcessHeap(), 0, newMatrix[i]);
    }
    HeapFree(GetProcessHeap(), 0, newMatrix);

    return det;
}

// Точка входа консольного приложения
int main() {
    // Задаём размерность матрицы
    int orderMatrix = 0; // Размерность матрицы
    printf("orderMatrix = ");
    scanf("%d", &orderMatrix);

    // Создание матрицы и выделение памяти
    double** matrix = (double**)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, orderMatrix * sizeof(double*)); // Матрица
    for (int i = 0; i < orderMatrix; ++i) {
        matrix[i] = (double*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, orderMatrix * sizeof(double));
    }

    matrix[0][0] = 4;    matrix[0][1] = -3;   matrix[0][2] = 6;     matrix[0][3] = -7;
    matrix[1][0] = -2;   matrix[1][1] = -1;   matrix[1][2] = 0;     matrix[1][3] = 2;
    matrix[2][0] = 4;    matrix[2][1] = -3;   matrix[2][2] = 4;     matrix[2][3] = -1;
    matrix[3][0] = 3;    matrix[3][1] = 2;    matrix[3][2] = 9;     matrix[3][3] = -4;

    printf_s("\n");

    matrix[0][0] = 4;    matrix[0][1] = -3;   matrix[0][2] = 6;     matrix[0][3] = -7;
    matrix[1][0] = -2;   matrix[1][1] = -1;   matrix[1][2] = 0;     matrix[1][3] = 2;
    matrix[2][0] = 4;    matrix[2][1] = -3;   matrix[2][2] = 4;     matrix[2][3] = -1;
    matrix[3][0] = 3;    matrix[3][1] = 2;    matrix[3][2] = 9;     matrix[3][3] = -4;

    printf_s("\n");

    matrix[0][0] = 4;    matrix[0][1] = -3;   matrix[0][2] = 6;     matrix[0][3] = -7;
    matrix[1][0] = -2;   matrix[1][1] = -1;   matrix[1][2] = 0;     matrix[1][3] = 2;
    matrix[2][0] = 4;    matrix[2][1] = -3;   matrix[2][2] = 4;     matrix[2][3] = -1;
    matrix[3][0] = 3;    matrix[3][1] = 2;    matrix[3][2] = 9;     matrix[3][3] = -4;

    printf_s("\n");

    // Ввод матрицы
    scanMatrix(matrix, orderMatrix);
    printf("\nOriginal matrix\n");
    printMatrix(matrix, orderMatrix);

    // Вычисляем определитель
    double det = chioDeterminant(matrix, orderMatrix);

    // Выводим результат в консоль
    printf("\nMatrix determinant: %lf\n", det);

    // Освобождаем память
    for (int i = 0; i < orderMatrix; ++i) HeapFree(GetProcessHeap(), 0, matrix[i]);
    HeapFree(GetProcessHeap(), 0, matrix);

    system("pause");

    return 0;
}