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
    return m[0][0] * m[1][1] - m[0][1] * m[1][0];  // ad - bc
}

// Ручная реализация pow
double customPow(double base, int exponent) {
    double result = 1.0;
    for (int i = 0; i < exponent; ++i) result *= base;
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
    tempMatrix[0][0] = data->matrix[data->row][data->col];
    tempMatrix[0][1] = data->matrix[data->row][data->col + 1];
    tempMatrix[1][0] = data->matrix[data->row + 1][data->col];
    tempMatrix[1][1] = data->matrix[data->row + 1][data->col + 1];

    // Вычисляем и сохраняем результат
    *(data->result) = det2(tempMatrix) / data->pivot;

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

    double pivot = matrix[pivotRow][pivotCol];
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

    // Освобождаем ресурсы
    for (int i = 0; i < threadCount; ++i) CloseHandle(threads[i]);
    HeapFree(GetProcessHeap(), 0, threads);
    HeapFree(GetProcessHeap(), 0, threadData);

    // Рекурсивный вызов, сделать n-3 и делить 1 на это
    double det = customPow(pivot, n - 2) * chioDeterminant(newMatrix, newSize);


    // Освобождаем память
    for (int i = 0; i < newSize; ++i) HeapFree(GetProcessHeap(), 0, newMatrix[i]);
    HeapFree(GetProcessHeap(), 0, newMatrix);

    return det;
}

// Точка входа консольного приложения
int main() {
    // Создаем тестовую матрицу 10x10
    const int n = 3;
    double** matrix = (double**)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, n * sizeof(double*));
    for (int i = 0; i < n; ++i) {
        matrix[i] = (double*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, n * sizeof(double));
    }

    // Заполняем матрицу (диагональная с шумом)
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            scanf_s("%lf", &matrix[i][j]);
        }
    }

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
    printf("Matrix determinant: %.4f\n", det);

    // Освобождаем память
    for (int i = 0; i < n; ++i) HeapFree(GetProcessHeap(), 0, matrix[i]);
    HeapFree(GetProcessHeap(), 0, matrix);

    return 0;
}