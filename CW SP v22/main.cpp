#include <windows.h>
#include <vector>
#include <cmath>
#include "resource.h"

#define M_PI 3.1415926535

// Определим интерфейс для всех фигур
class Shape {
public:
    virtual void draw(HDC hdc) = 0;
    virtual void move(int dx, int dy) = 0;
    virtual Shape* copy() const = 0;
    virtual void rotate(double angle) = 0;
    virtual void mirror(bool vertical) = 0;

    // Добавляем виртуальный метод isClicked
    virtual bool isClicked(int x, int y) = 0;

    virtual ~Shape() {}  // Виртуальный деструктор для безопасного удаления производных классов
};

// Точка
class Point : public Shape {
public:
    int x, y;

    Point() : x(0), y(0) {}  // Конструктор по умолчанию

    Point(int x, int y) : x(x), y(y) {}

    void draw(HDC hdc) override {
        SetPixel(hdc, x, y, RGB(0, 0, 0));
    }

    void move(int dx, int dy) override {
        x += dx;
        y += dy;
    }

    Shape* copy() const override {
        return new Point(x, y);
    }

    void rotate(double angle) override {
        // Поворот точки не имеет смысла
    }

    void mirror(bool vertical) override {
        if (vertical) {
            x = -x;
        }
        else {
            y = -y;
        }
    }

    bool isClicked(int x, int y) override {
        return this->x == x && this->y == y; // Простая проверка
    }
};

// Линия (отрезок)
class Line : public Shape {
protected:
    Point start, end;
public:
    Line(Point start, Point end) : start(start), end(end) {}

    void draw(HDC hdc) override {
        MoveToEx(hdc, start.x, start.y, NULL);
        LineTo(hdc, end.x, end.y);
    }

    bool isClicked(int x, int y) {
        // Простая проверка на попадание в линию (с учётом некоторой погрешности)
        int tolerance = 5;
        int dx = end.x - start.x;
        int dy = end.y - start.y;
        double distance = std::abs(dy * x - dx * y + end.x * start.y - end.y * start.x) / sqrt(dx * dx + dy * dy);
        return distance < tolerance;
    }

    void move(int dx, int dy) override {
        start.move(dx, dy);
        end.move(dx, dy);
    }

    Shape* copy() const override {
        return new Line(start, end);
    }

    void rotate(double angle) override {
        // Реализация поворота линии
    }

    void mirror(bool vertical) override {
        start.mirror(vertical);
        end.mirror(vertical);
    }
};

// Круг
class Circle : public Shape {
protected:
    Point center;
    int radius;

public:
    Circle(Point center, int radius) : center(center), radius(radius) {}

    // Методы доступа
    Point getCenter() const { return center; }
    int getRadius() const { return radius; }

    void draw(HDC hdc) override {
        Ellipse(hdc, center.x - radius, center.y - radius, center.x + radius, center.y + radius);
    }

    bool isClicked(int x, int y) {
        int dx = x - center.x;
        int dy = y - center.y;
        return (dx * dx + dy * dy <= radius * radius);
    }

    void move(int dx, int dy) override {
        center.move(dx, dy);
    }

    Shape* copy() const override {
        return new Circle(center, radius);
    }

    void rotate(double angle) override {
        // Поворот круга не имеет смысла
    }

    void mirror(bool vertical) override {
        center.mirror(vertical);
    }
};

namespace MyArc {
    class Arc : public Shape {
    public:
        Point center;
        int radius;
        double startAngle, endAngle;  // Углы в радианах

        // Конструктор с центром, радиусом и углами
        Arc(Point center, int radius, double startAngle, double endAngle)
            : center(center), radius(radius), startAngle(startAngle), endAngle(endAngle) {}

        // Конструктор с центром и двумя конечными точками
        Arc(Point center, Point startPoint, Point endPoint)
            : center(center) {
            radius = sqrt(pow(startPoint.x - center.x, 2) + pow(startPoint.y - center.y, 2));
            startAngle = atan2(startPoint.y - center.y, startPoint.x - center.x);
            endAngle = atan2(endPoint.y - center.y, endPoint.x - center.x);
        }

        // Метод для рисования дуги
        void draw(HDC hdc) override {
            // Преобразуем углы в координаты точек на окружности
            int xStart = center.x + radius * cos(startAngle);
            int yStart = center.y + radius * sin(startAngle);
            int xEnd = center.x + radius * cos(endAngle);
            int yEnd = center.y + radius * sin(endAngle);

            // Вызов функции Arc из WinAPI для рисования дуги
            ::Arc(hdc, center.x - radius, center.y - radius, center.x + radius, center.y + radius,
                xStart, yStart, xEnd, yEnd);
        }

        void move(int dx, int dy) override {
            center.move(dx, dy);
        }

        Shape* copy() const override {
            return new Arc(center, radius, startAngle, endAngle);
        }

        void rotate(double angle) override {
            // Логика вращения дуги (при необходимости)
        }

        void mirror(bool vertical) override {
            if (vertical) {
                startAngle = -startAngle;
                endAngle = -endAngle;
            }
            else {
                startAngle = M_PI - startAngle;
                endAngle = M_PI - endAngle;
            }
        }

        // Проверка клика на дуге
        bool isClicked(int x, int y) override {
            int dx = x - center.x;
            int dy = y - center.y;

            // Проверяем, находится ли точка в пределах радиуса
            if (dx * dx + dy * dy <= radius * radius) {
                // Вычисляем угол точки относительно центра дуги
                double angle = atan2(dy, dx);

                // Приводим углы к диапазону от 0 до 2*PI для удобства
                double normalizedStartAngle = fmod(startAngle + 2 * M_PI, 2 * M_PI);
                double normalizedEndAngle = fmod(endAngle + 2 * M_PI, 2 * M_PI);
                double normalizedAngle = fmod(angle + 2 * M_PI, 2 * M_PI);

                // Проверяем, находится ли угол между startAngle и endAngle
                if (normalizedStartAngle < normalizedEndAngle) {
                    return (normalizedAngle >= normalizedStartAngle && normalizedAngle <= normalizedEndAngle);
                }
                else { // Обработка случаев, когда дуга пересекает 0 радиан (например, от 350° до 10°)
                    return (normalizedAngle >= normalizedStartAngle || normalizedAngle <= normalizedEndAngle);
                }
            }
            return false; // Точка вне радиуса
        }

    };
}

class Ring : public Shape {
public:
    Circle outerCircle;
    Circle innerCircle;

    Ring(Point center, int outerRadius, int innerRadius)
        : outerCircle(center, outerRadius), innerCircle(center, innerRadius) {}

    void draw(HDC hdc) override {
        outerCircle.draw(hdc);
        innerCircle.draw(hdc);
    }

    void move(int dx, int dy) override {
        outerCircle.move(dx, dy);
        innerCircle.move(dx, dy);
    }

    Shape* copy() const override {
        return new Ring(outerCircle.getCenter(), outerCircle.getRadius(), innerCircle.getRadius());
    }

    void rotate(double angle) override {
        // Кольцо не имеет смысла вращать
    }

    void mirror(bool vertical) override {
        outerCircle.mirror(vertical);
        innerCircle.mirror(vertical);
    }

    bool isClicked(int x, int y) override {
        int dx = x - outerCircle.getCenter().x;
        int dy = y - outerCircle.getCenter().y;

        // Проверяем, находится ли точка внутри внешнего круга и снаружи внутреннего
        bool insideOuter = (dx * dx + dy * dy <= outerCircle.getRadius() * outerCircle.getRadius());
        bool insideInner = (dx * dx + dy * dy <= innerCircle.getRadius() * innerCircle.getRadius());

        return insideOuter && !insideInner; // Внутри внешнего и снаружи внутреннего
    }
};

class Polyline : public Shape {
public:
    std::vector<Point> points;

    Polyline(const std::vector<Point>& points) : points(points) {}

    void draw(HDC hdc) override {
        for (size_t i = 0; i < points.size() - 1; ++i) {
            MoveToEx(hdc, points[i].x, points[i].y, NULL);
            LineTo(hdc, points[i + 1].x, points[i + 1].y);
        }
    }

    void move(int dx, int dy) override {
        for (Point& p : points) {
            p.move(dx, dy);
        }
    }

    Shape* copy() const override {
        return new Polyline(points);
    }

    void rotate(double angle) override {
        // Логика вращения ломаной
    }

    void mirror(bool vertical) override {
        for (Point& p : points) {
            p.mirror(vertical);
        }
    }

    bool isClicked(int x, int y) override {
        int tolerance = 5; // Допустимое расстояние от линии

        for (size_t i = 0; i < points.size() - 1; ++i) {
            int dx = points[i + 1].x - points[i].x;
            int dy = points[i + 1].y - points[i].y;

            // Уравнение линии: Ax + By + C = 0
            double A = dy;
            double B = -dx;
            double C = dx * points[i].y - dy * points[i].x;

            // Расстояние от точки до линии
            double distance = std::abs(A * x + B * y + C) / std::sqrt(A * A + B * B);

            if (distance < tolerance) {
                return true; // Клик на линии
            }
        }
        return false; // Не попал в полилинию
    }
};

class Polygon : public Polyline {
public:
    Polygon(const std::vector<Point>& points) : Polyline(points) {}

    void draw(HDC hdc) override {
        Polyline::draw(hdc);
        MoveToEx(hdc, points.back().x, points.back().y, NULL);
        LineTo(hdc, points[0].x, points[0].y);
    }

    Shape* copy() const override {
        return new Polygon(points);
    }

    bool isClicked(int x, int y) override {
        bool inside = false;

        for (size_t i = 0, j = points.size() - 1; i < points.size(); j = i++) {
            if (((points[i].y > y) != (points[j].y > y)) &&
                (x < (points[j].x - points[i].x) * (y - points[i].y) / (points[j].y - points[i].y) + points[i].x)) {
                inside = !inside;
            }
        }
        return inside; // Внутри многоугольника
    }
};

class Triangle : public Polygon {
public:
    Triangle(Point p1, Point p2, Point p3) : Polygon({ p1, p2, p3 }) {}
};

class Parallelogram : public Polygon {
public:
    Parallelogram(Point p1, Point p2, Point p3, Point p4)
        : Polygon({ p1, p2, p3, p4 }) {}
};

// Основная логика для окна
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static std::vector<Shape*> shapes;
    static Shape* selectedShape = nullptr;

    enum Mode {
        MODE_SELECT,
        MODE_ADD_LINE_FIRST_POINT,
        MODE_ADD_LINE_SECOND_POINT,
        MODE_ADD_CIRCLE_FIRST_POINT,
        MODE_ADD_CIRCLE_SECOND_POINT,
        MODE_ADD_ARC_FIRST_POINT,
        MODE_ADD_ARC_SECOND_POINT,
        MODE_ADD_RING_FIRST_POINT,
        MODE_ADD_RING_SECOND_POINT,
        MODE_ADD_POLYLINE_FIRST_POINT,
        MODE_ADD_POLYGON_FIRST_POINT,  
        MODE_ADD_TRIANGLE_FIRST_POINT, 
        MODE_ADD_PARALLELOGRAM_FIRST_POINT
    };


    static Mode mode = MODE_SELECT;

    static Point startPoint, endPoint;
    HDC hdc;
    PAINTSTRUCT ps;

    switch (msg) {
    case WM_CREATE:
    {
        // Загружаем меню из ресурса
        HMENU hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_MENU1));
        SetMenu(hwnd, hMenu);
    }
    break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_ADD_LINE:
            mode = MODE_ADD_LINE_FIRST_POINT;
            break;
        case IDM_ADD_CIRCLE:
            mode = MODE_ADD_CIRCLE_FIRST_POINT;
            break;
        case IDM_ADD_ARC:
            mode = MODE_ADD_ARC_FIRST_POINT;
            break;
        case IDM_ADD_RING:
            mode = MODE_ADD_RING_FIRST_POINT;
            break;
        case IDM_SELECT_MODE:
            mode = MODE_SELECT;
            break;
        case IDM_MIRROR_VERTICAL:  // Обработка зеркального отображения
            if (selectedShape) {
                selectedShape->mirror(true); // Вертикальное отражение
                InvalidateRect(hwnd, NULL, TRUE); // Обновляем окно
            }
            break;
        case IDM_MIRROR_HORIZONTAL:  // Обработка зеркального отображения
            if (selectedShape) {
                selectedShape->mirror(false); // Вертикальное отражение
                InvalidateRect(hwnd, NULL, TRUE); // Обновляем окно
            }
            break;
        case IDM_ADD_POLYLINE:
            mode = MODE_ADD_POLYLINE_FIRST_POINT; // Выбор режима создания Polyline
            break;
        case IDM_ADD_POLYGON:
            mode = MODE_ADD_POLYGON_FIRST_POINT; // Выбор режима создания Polygon
            break;
        case IDM_ADD_TRIANGLE:
            mode = MODE_ADD_TRIANGLE_FIRST_POINT; // Выбор режима создания Triangle
            break;
        case IDM_ADD_PARALLELOGRAM:
            mode = MODE_ADD_PARALLELOGRAM_FIRST_POINT; // Выбор режима создания Parallelogram
            break;

        }
        break;

    case WM_LBUTTONDOWN:
    {
        int xPos = LOWORD(lParam);
        int yPos = HIWORD(lParam);

        switch (mode) {
        case MODE_SELECT:
            selectedShape = nullptr;
            for (Shape* shape : shapes) {
                if (shape->isClicked(xPos, yPos)) {
                    selectedShape = shape;
                    break;
                }
            }
            break;

        case MODE_ADD_LINE_FIRST_POINT:
            startPoint = Point(xPos, yPos);
            mode = MODE_ADD_LINE_SECOND_POINT;
            break;

        case MODE_ADD_LINE_SECOND_POINT:
            endPoint = Point(xPos, yPos);
            shapes.push_back(new Line(startPoint, endPoint));
            mode = MODE_SELECT;
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case MODE_ADD_CIRCLE_FIRST_POINT:
            startPoint = Point(xPos, yPos);
            mode = MODE_ADD_CIRCLE_SECOND_POINT;
            break;

        case MODE_ADD_CIRCLE_SECOND_POINT:
        {
            int radius = sqrt(pow(xPos - startPoint.x, 2) + pow(yPos - startPoint.y, 2));
            shapes.push_back(new Circle(startPoint, radius));
            mode = MODE_SELECT;
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }

        case MODE_ADD_ARC_FIRST_POINT:
            startPoint = Point(xPos, yPos);
            mode = MODE_ADD_ARC_SECOND_POINT;
            break;

        case MODE_ADD_ARC_SECOND_POINT:
        {
            endPoint = Point(xPos, yPos);
            int radiusArc = sqrt(pow(startPoint.x - endPoint.x, 2) + pow(startPoint.y - endPoint.y, 2)); // Расчет радиуса
            shapes.push_back(new MyArc::Arc(startPoint, radiusArc, 45 * M_PI / 180, 135 * M_PI / 180)); // Пример углов в радианах
            mode = MODE_SELECT;
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }

        case MODE_ADD_RING_FIRST_POINT:
            startPoint = Point(xPos, yPos);
            mode = MODE_ADD_RING_SECOND_POINT;
            break;

        case MODE_ADD_RING_SECOND_POINT:
        {
            int outerRadius = sqrt(pow(xPos - startPoint.x, 2) + pow(yPos - startPoint.y, 2));
            shapes.push_back(new Ring(startPoint, outerRadius, outerRadius / 2)); // Пример кольца
            mode = MODE_SELECT;
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }



        }

        break;
    }

    case WM_KEYDOWN:
        if (selectedShape) {
            int moveDistance = 10;

            switch (wParam) {
            case VK_LEFT:
                selectedShape->move(-moveDistance, 0);
                break;
            case VK_RIGHT:
                selectedShape->move(moveDistance, 0);
                break;
            case VK_UP:
                selectedShape->move(0, -moveDistance);
                break;
            case VK_DOWN:
                selectedShape->move(0, moveDistance);
                break;
            }
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;

    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        for (Shape* shape : shapes) {
            shape->draw(hdc);
        }
        EndPaint(hwnd, &ps);
        break;

    case WM_DESTROY:
        for (Shape* shape : shapes) {
            delete shape;
        }
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// Основная функция
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "MyWindowClass";

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    HWND hwnd = CreateWindowEx(0, "MyWindowClass", "Примитивный графический редактор",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Инициализация фигур
    std::vector<Shape*> shapes;
    shapes.push_back(new Circle(Point(200, 200), 100));
    shapes.push_back(new Line(Point(100, 100), Point(300, 300)));

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}
