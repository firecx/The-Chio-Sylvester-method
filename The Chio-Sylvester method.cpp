#include <Windows.h>  // Для HeapAlloc/HeapFree и многопоточности
#include <iostream>

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
    return (m[0][0] * m[1][1]) - (m[0][1] * m[1][0]);  // ad - bc
}

// Ручная реализация pow
double pow(double base, int exponent) {
    double result = 1.0;
    if (exponent >= 0) {
        for (int i = 0; i < exponent; ++i) result *= base;
    }
    else
    {
        for (int i = 0; i < exponent; ++i) result /= base;
    }
    return result;
}

// Функция потока для вычисления элемента
DWORD WINAPI computeElement(LPVOID lpParam) {
    ThreadData* data = (ThreadData*)lpParam;

    // Создаем временную матрицу 2x2
    double** tempMatrix = (double**)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 2 * sizeof(double*));
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

// Основная функция вычисления определителя
double chioDeterminant(double** matrix, int n) {
    if (n == 1) return matrix[0][0];
    if (n == 2) return det2(matrix);

    // Поиск опорного элемента
    /*
    int pivotRow = 0, pivotCol = 0;
    BOOL found = FALSE;
    for (int i = 0; i < n && !found; ++i) {
        for (int j = 0; j < n && !found; ++j) {
            if (matrix[i][j] != 0) {
                pivotRow = i;
                pivotCol = j;
                found = TRUE;
            }
        }
    }
    if (!found) return 0;
    */

    double pivot = matrix[0][0];
    int newSize = n - 1;

    // Выделяем память под новую матрицу
    double** newMatrix = (double**)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, newSize * sizeof(double*));
    for (int i = 0; i < newSize; ++i) {
        newMatrix[i] = (double*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, newSize * sizeof(double));
    }

    // Подготавливаем многопоточное вычисление
    HANDLE* threads = (HANDLE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, newSize * newSize * sizeof(HANDLE));
    ThreadData* threadData = (ThreadData*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, newSize * newSize * sizeof(ThreadData));

    // Запускаем потоки
    int threadCount = 0;
    for (int i = 0; i < newSize; ++i) {
        for (int j = 0; j < newSize; ++j) {
            threadData[threadCount] = { matrix, n, pivot, i, j, &newMatrix[i][j] };
            threads[threadCount] = CreateThread(NULL, 0, computeElement, &threadData[threadCount], 0, NULL);
            ++threadCount;
        }
    }

    // Ожидаем завершения потоков
    WaitForMultipleObjects(threadCount, threads, TRUE, INFINITE);

    for (int i = 0; i < newSize; ++i) {
        for (int j = 0; j < newSize; ++j) {
            
        }
    }

    // Освобождаем ресурсы
    for (int i = 0; i < threadCount; ++i) CloseHandle(threads[i]);
    HeapFree(GetProcessHeap(), 0, threads);
    HeapFree(GetProcessHeap(), 0, threadData);

    printf_s("\nnewMatrix\n");

    for (int i = 0; i < newSize; ++i) {
        for (int j = 0; j < newSize; ++j) {
            printf_s("%lf ", newMatrix[i][j]);
        }
        printf_s("\n");
    }
    printf("1/%f\n", pow(pivot, n - 2));
    // Рекурсивный вызов
    double det = chioDeterminant(newMatrix, newSize) / pow(pivot, n-2);


    // Освобождаем память
    for (int i = 0; i < newSize; ++i) HeapFree(GetProcessHeap(), 0, newMatrix[i]);
    HeapFree(GetProcessHeap(), 0, newMatrix);

    return det;
}

// Точка входа консольного приложения
int main() {
    // Создаем тестовую матрицу 10x10
    const int n = 4;
    double** matrix = (double**)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, n * sizeof(double*));
    for (int i = 0; i < n; ++i) {
        matrix[i] = (double*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, n * sizeof(double));
    }

    // Заполняем матрицу (диагональная с шумом)
    /*
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            scanf_s("%lf", &matrix[i][j]);
        }
    }
    */

    matrix[0][0] = 4;    matrix[0][1] = -3;   matrix[0][2] = 6;     matrix[0][3] = -7;
    matrix[1][0] = -2;   matrix[1][1] = -1;   matrix[1][2] = 0;     matrix[1][3] = 2;
    matrix[2][0] = 4;    matrix[2][1] = -3;   matrix[2][2] = 4;     matrix[2][3] = -1;
    matrix[3][0] = 3;    matrix[3][1] = 2;    matrix[3][2] = 9;     matrix[3][3] = -4;

    printf_s("\n");

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            printf_s("%lf ", matrix[i][j]);
        }
        printf_s("\n");
    }

    // Вычисляем определитель
    double det = chioDeterminant(matrix, n);

    // Выводим результат в консоль
    char buffer[50];
    sprintf_s(buffer, "Determinant: %.4f", det);
    OutputDebugStringA(buffer);  // Для отладки
    printf("\nMatrix determinant: %.4f\n", det);

    // Освобождаем память
    for (int i = 0; i < n; ++i) HeapFree(GetProcessHeap(), 0, matrix[i]);
    HeapFree(GetProcessHeap(), 0, matrix);

    return 0;
}