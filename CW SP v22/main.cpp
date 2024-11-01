#include <windows.h>
#include <vector>
#include <cmath>
#include "resource.h"

#define M_PI 3.1415926535

namespace MyShapes {
    // Определим интерфейс для всех фигур
    class Shape {
    protected:
        COLORREF color;
    public:

        virtual void setColor(COLORREF newColor) {
            color = newColor;
        }

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
            SetPixel(hdc, x, y, color);
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

        void rotateAround(const Point& center, double angle) {
            double rad = angle * M_PI / 180.0; // Переводим угол в радианы
            double cosAngle = cos(rad);
            double sinAngle = sin(rad);

            // Смещаем точку к началу координат
            double dx = x - center.x;
            double dy = y - center.y;

            // Поворачиваем и возвращаем точку обратно
            x = center.x + (dx * cosAngle - dy * sinAngle);
            y = center.y + (dx * sinAngle + dy * cosAngle);
        }
    };

    // Линия (отрезок)
    class Line : public Shape {
    protected:
        Point start, end;
    public:
        Line(Point start, Point end) : start(start), end(end) {}

        void draw(HDC hdc) override {
            HPEN pen = CreatePen(PS_SOLID, 1, color); // Создаем перо выбранного цвета
            HPEN oldPen = (HPEN)SelectObject(hdc, pen);

            MoveToEx(hdc, start.x, start.y, NULL);
            LineTo(hdc, end.x, end.y);

            SelectObject(hdc, oldPen); // Восстанавливаем старое перо
            DeleteObject(pen);          // Удаляем созданное перо
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
            // Находим центр линии
            Point center((start.x + end.x) / 2, (start.y + end.y) / 2);

            // Поворачиваем обе точки вокруг центра линии
            start.rotateAround(center, angle);
            end.rotateAround(center, angle);
        }

        void mirror(bool vertical) override {

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
            HPEN pen = CreatePen(PS_SOLID, 1, color); // Создаем перо выбранного цвета
            HPEN oldPen = (HPEN)SelectObject(hdc, pen);

            Ellipse(hdc, center.x - radius, center.y - radius, center.x + radius, center.y + radius);

            SelectObject(hdc, oldPen); // Восстанавливаем старое перо
            DeleteObject(pen);          // Удаляем созданное перо
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

        }
    };

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
            HPEN pen = CreatePen(PS_SOLID, 1, color); // Создаем перо выбранного цвета
            HPEN oldPen = (HPEN)SelectObject(hdc, pen);

            // Преобразуем углы в координаты точек на окружности
            int xStart = center.x + radius * cos(startAngle);
            int yStart = center.y + radius * sin(startAngle);
            int xEnd = center.x + radius * cos(endAngle);
            int yEnd = center.y + radius * sin(endAngle);

            // Вызов функции Arc из WinAPI для рисования дуги
            ::Arc(hdc, center.x - radius, center.y - radius, center.x + radius, center.y + radius,
                xStart, yStart, xEnd, yEnd);

            SelectObject(hdc, oldPen); // Восстанавливаем старое перо
            DeleteObject(pen);          // Удаляем созданное перо
        }

        void move(int dx, int dy) override {
            center.move(dx, dy);
        }

        Shape* copy() const override {
            return new Arc(center, radius, startAngle, endAngle);
        }

        void rotate(double angle) override {
            startAngle += angle;
            endAngle += angle;

            // Приводим углы к диапазону от 0 до 2π для корректного отображения
            startAngle = fmod(startAngle + 2 * M_PI, 2 * M_PI);
            endAngle = fmod(endAngle + 2 * M_PI, 2 * M_PI);
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

    class Ring : public Shape {
    public:
        Circle outerCircle;
        Circle innerCircle;

        Ring(Point center, int outerRadius, int innerRadius)
            : outerCircle(center, outerRadius), innerCircle(center, innerRadius) {}

        void setColor(COLORREF newColor) {
            outerCircle.setColor(newColor);
            innerCircle.setColor(newColor);
        }

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
            HPEN pen = CreatePen(PS_SOLID, 1, color); // Создаем перо выбранного цвета
            HPEN oldPen = (HPEN)SelectObject(hdc, pen);

            for (size_t i = 0; i < points.size() - 1; ++i) {
                MoveToEx(hdc, points[i].x, points[i].y, NULL);
                LineTo(hdc, points[i + 1].x, points[i + 1].y);
            }

            SelectObject(hdc, oldPen); // Восстанавливаем старое перо
            DeleteObject(pen);          // Удаляем созданное перо
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
            if (points.empty()) return;

            // Находим центр как среднее всех точек
            double centerX = 0, centerY = 0;
            for (const MyShapes::Point& p : points) {
                centerX += p.x;
                centerY += p.y;
            }
            centerX /= points.size();
            centerY /= points.size();
            MyShapes::Point center(centerX, centerY);

            // Поворачиваем каждую точку вокруг центра
            for (MyShapes::Point& point : points) {
                point.rotateAround(center, angle);
            }
        }

        void mirror(bool vertical) override {
            if (points.empty()) return;

            // Находим центр ломаной как среднее всех точек
            double centerX = 0, centerY = 0;
            for (const Point& p : points) {
                centerX += p.x;
                centerY += p.y;
            }
            centerX /= points.size();
            centerY /= points.size();
            Point center(centerX, centerY);

            // Зеркально отражаем каждую точку относительно центра
            for (Point& p : points) {
                if (vertical) {
                    p.x = center.x - (p.x - center.x);
                }
                else {
                    p.y = center.y - (p.y - center.y);
                }
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
            HPEN pen = CreatePen(PS_SOLID, 1, color); // Создаем перо выбранного цвета
            HPEN oldPen = (HPEN)SelectObject(hdc, pen);

            Polyline::draw(hdc);
            MoveToEx(hdc, points.back().x, points.back().y, NULL);
            LineTo(hdc, points[0].x, points[0].y);

            SelectObject(hdc, oldPen); // Восстанавливаем старое перо
            DeleteObject(pen);          // Удаляем созданное перо
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

        void rotate(double angle) override {
            // Находим центр треугольника как среднее всех точек
            Point center(
                (points[0].x + points[1].x + points[2].x) / 3,
                (points[0].y + points[1].y + points[2].y) / 3
            );

            // Поворачиваем каждую точку вокруг центра
            for (Point& point : points) {
                point.rotateAround(center, angle);
            }
        }
    };

    class Parallelogram : public Polygon {
    public:
        Parallelogram(Point p1, Point p2, double angle) : Polygon({ p1, p2 }) {
            // Вычисляем третью и четвертую точку на основе угла
            double dx = p2.x - p1.x;
            double dy = p2.y - p1.y;

            // Длина стороны
            double length = sqrt(dx * dx + dy * dy);

            // Угол наклона (в радианах)
            double rad = angle * M_PI / 180.0;

            // Вычисляем третью точку, используя угол наклона
            Point p3(p1.x + length * cos(rad), p1.y + length * sin(rad));
            Point p4(p2.x + length * cos(rad), p2.y + length * sin(rad));

            points.push_back(p4);
            points.push_back(p3);
        }

        void rotate(double angle) override {
            Point center(
                (points[0].x + points[1].x + points[2].x + points[3].x) / 4,
                (points[0].y + points[1].y + points[2].y + points[3].y) / 4
            );

            for (Point& point : points) {
                point.rotateAround(center, angle);
            }
        }
    };

}

// Функция для показа диалога и получения количества точек
int ShowPointDialog(HWND hwnd) {
    INT_PTR ret = DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG_POINTS), hwnd, [](HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) -> INT_PTR {
        switch (message) {
        case WM_INITDIALOG:
            return (INT_PTR)TRUE;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {
                char buffer[256];
                GetDlgItemText(hDlg, IDC_EDIT_POINTS, buffer, 256);
                int numPoints = atoi(buffer);
                EndDialog(hDlg, numPoints);
                return (INT_PTR)TRUE;
            }
            else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, 0);
                return (INT_PTR)TRUE;
            }
            break;
        }
        return (INT_PTR)FALSE;
        });

    return ret; // Возвращаем количество точек
}

int ShowAngleDialog(HWND hwnd) {
    INT_PTR ret = DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG_ANGLE), hwnd, [](HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) -> INT_PTR {
        switch (message) {
        case WM_INITDIALOG:
            return (INT_PTR)TRUE;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {
                char buffer[256];
                GetDlgItemText(hDlg, IDC_EDIT_ANGLE, buffer, 256);
                int numPoints = atoi(buffer);
                EndDialog(hDlg, numPoints);
                return (INT_PTR)TRUE;
            }
            else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, 0);
                return (INT_PTR)TRUE;
            }
            break;
        }
        return (INT_PTR)FALSE;
        });

    return ret; // Возвращаем количество точек
}

// Основная логика для окна
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HMENU hContextMenu;

    static std::vector<MyShapes::Shape*> shapes;
    static MyShapes::Shape* selectedShape = nullptr;

    static int numPoints = 0;
    static std::vector<MyShapes::Point> points;

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

    static MyShapes::Point startPoint, endPoint;
    HDC hdc;
    PAINTSTRUCT ps;

    // Флаги отображения для каждой фигуры
    static bool showLines = true;
    static bool showCircles = true;
    static bool showArcs = true;
    static bool showRings = true;
    static bool showPolylines = true;
    static bool showPolygons = true;
    static bool showTriangles = true;
    static bool showParallelograms = true;

    switch (msg) {
    case WM_CREATE:
    {
        // Загружаем меню из ресурса
        HMENU hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_MENU1));
        SetMenu(hwnd, hMenu);

        // Загружаем контекстное меню
        hContextMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_CONTEXT_MENU));
        if (hContextMenu)
            hContextMenu = GetSubMenu(hContextMenu, 0);
    }
    break;

    case WM_COMMAND:
    {
        // Общая обработка для всех фигур, которые требуют диалог ввода точек
        bool shapeRequiresPoints = false;
        Mode newMode;

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
            newMode = MODE_ADD_POLYLINE_FIRST_POINT;
            shapeRequiresPoints = true;
            break;

        case IDM_ADD_POLYGON:
            newMode = MODE_ADD_POLYGON_FIRST_POINT;
            shapeRequiresPoints = true;
            break;

        case IDM_ADD_TRIANGLE:
            mode = MODE_ADD_TRIANGLE_FIRST_POINT;
            break;

        case IDM_ADD_PARALLELOGRAM:
            mode = MODE_ADD_PARALLELOGRAM_FIRST_POINT;
            break;

        case IDM_SHOW_LINES:
            showLines = !showLines;
            CheckMenuItem(GetMenu(hwnd), IDM_SHOW_LINES, showLines ? MF_CHECKED : MF_UNCHECKED);
            InvalidateRect(hwnd, NULL, TRUE); // Перерисовать окно
            break;
        case IDM_SHOW_CIRCLES:
            showCircles = !showCircles;
            CheckMenuItem(GetMenu(hwnd), IDM_SHOW_CIRCLES, showCircles ? MF_CHECKED : MF_UNCHECKED);
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        case IDM_SHOW_ARCS:
            showArcs = !showArcs;
            CheckMenuItem(GetMenu(hwnd), IDM_SHOW_ARCS, showArcs ? MF_CHECKED : MF_UNCHECKED);
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        case IDM_SHOW_RINGS:
            showRings = !showRings;
            CheckMenuItem(GetMenu(hwnd), IDM_SHOW_RINGS, showRings ? MF_CHECKED : MF_UNCHECKED);
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        case IDM_SHOW_POLYLINES:
            showPolylines = !showPolylines;
            CheckMenuItem(GetMenu(hwnd), IDM_SHOW_POLYLINES, showPolylines ? MF_CHECKED : MF_UNCHECKED);
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        case IDM_SHOW_POLYGONS:
            showPolygons = !showPolygons;
            CheckMenuItem(GetMenu(hwnd), IDM_SHOW_POLYGONS, showPolygons ? MF_CHECKED : MF_UNCHECKED);
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        case IDM_SHOW_TRIANGLES:
            showTriangles = !showTriangles;
            CheckMenuItem(GetMenu(hwnd), IDM_SHOW_TRIANGLES, showTriangles ? MF_CHECKED : MF_UNCHECKED);
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        case IDM_SHOW_PARALLELOGRAMS:
            showParallelograms = !showParallelograms;
            CheckMenuItem(GetMenu(hwnd), IDM_SHOW_PARALLELOGRAMS, showParallelograms ? MF_CHECKED : MF_UNCHECKED);
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        case IDM_ROTATE_SELECTED:
            if (selectedShape) {
                selectedShape->rotate(10); // Вращаем на 15 градусов
                InvalidateRect(hwnd, NULL, TRUE); // Обновляем окно
            }
            break;
        }

        if (shapeRequiresPoints) {
            numPoints = ShowPointDialog(hwnd); // Показываем диалог для ввода количества точек
            if (numPoints > 0) {
                points.clear(); // Очищаем статический массив точек
                mode = newMode; // Устанавливаем режим для добавления точек
            }
            else {
                mode = MODE_SELECT; // Возвращаемся в режим выбора, если количество точек не введено
            }
        }

        break;
    }

    case WM_LBUTTONDOWN:
    {
        int xPos = LOWORD(lParam);
        int yPos = HIWORD(lParam);

        switch (mode) {
        case MODE_SELECT:

            if (selectedShape != nullptr)
                selectedShape->setColor(RGB(0, 0, 0));

            selectedShape = nullptr;

            for (MyShapes::Shape* shape : shapes) {
                if (shape->isClicked(xPos, yPos)) {
                    selectedShape = shape;
                    selectedShape->setColor(RGB(0, 0, 255));
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;
                }
            }
            break;

        case MODE_ADD_LINE_FIRST_POINT:
            startPoint = MyShapes::Point(xPos, yPos);
            mode = MODE_ADD_LINE_SECOND_POINT;
            break;

        case MODE_ADD_LINE_SECOND_POINT:
            endPoint = MyShapes::Point(xPos, yPos);
            shapes.push_back(new MyShapes::Line(startPoint, endPoint));
            mode = MODE_SELECT;
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case MODE_ADD_CIRCLE_FIRST_POINT:
            startPoint = MyShapes::Point(xPos, yPos);
            mode = MODE_ADD_CIRCLE_SECOND_POINT;
            break;

        case MODE_ADD_CIRCLE_SECOND_POINT:
        {
            int radius = sqrt(pow(xPos - startPoint.x, 2) + pow(yPos - startPoint.y, 2));
            shapes.push_back(new MyShapes::Circle(startPoint, radius));
            mode = MODE_SELECT;
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }

        case MODE_ADD_ARC_FIRST_POINT:
            startPoint = MyShapes::Point(xPos, yPos);
            mode = MODE_ADD_ARC_SECOND_POINT;
            break;

        case MODE_ADD_ARC_SECOND_POINT:
        {
            endPoint = MyShapes::Point(xPos, yPos);
            int radiusArc = sqrt(pow(startPoint.x - endPoint.x, 2) + pow(startPoint.y - endPoint.y, 2)); // Расчет радиуса
            shapes.push_back(new MyShapes::Arc(startPoint, radiusArc, 45 * M_PI / 180, 135 * M_PI / 180)); // Пример углов в радианах
            mode = MODE_SELECT;
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }

        case MODE_ADD_RING_FIRST_POINT:
            startPoint = MyShapes::Point(xPos, yPos);
            mode = MODE_ADD_RING_SECOND_POINT;
            break;

        case MODE_ADD_RING_SECOND_POINT:
        {
            int outerRadius = sqrt(pow(xPos - startPoint.x, 2) + pow(yPos - startPoint.y, 2));
            shapes.push_back(new MyShapes::Ring(startPoint, outerRadius, outerRadius / 2)); // Пример кольца
            mode = MODE_SELECT;
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }

        // Polyline
        case MODE_ADD_POLYLINE_FIRST_POINT:
            points.push_back(MyShapes::Point(xPos, yPos));
            if (points.size() == numPoints) {
                shapes.push_back(new MyShapes::Polyline(points));
                points.clear();
                mode = MODE_SELECT;
                InvalidateRect(hwnd, NULL, TRUE);
            }
            break;
            // Polygon
        case MODE_ADD_POLYGON_FIRST_POINT:
            points.push_back(MyShapes::Point(xPos, yPos));
            if (points.size() == numPoints) {
                shapes.push_back(new MyShapes::Polygon(points));
                points.clear();
                mode = MODE_SELECT;
                InvalidateRect(hwnd, NULL, TRUE);
            }
            break;
            // Triangle
        case MODE_ADD_TRIANGLE_FIRST_POINT:
            points.push_back(MyShapes::Point(xPos, yPos));
            if (points.size() == 3) {
                shapes.push_back(new MyShapes::Triangle(points[0], points[1], points[2]));
                points.clear();
                mode = MODE_SELECT;
                InvalidateRect(hwnd, NULL, TRUE);
            }
            break;
            // Parallelogram
        case MODE_ADD_PARALLELOGRAM_FIRST_POINT:
            points.push_back(MyShapes::Point(xPos, yPos));
            if (points.size() == 2) {
                double angle = ShowAngleDialog(hwnd);
                shapes.push_back(new MyShapes::Parallelogram(points[0], points[1], angle));
                points.clear();
                mode = MODE_SELECT;
                InvalidateRect(hwnd, NULL, TRUE);
            }
            break;

        }

        break;
    }

    case WM_RBUTTONDOWN: {
        // Получаем координаты клика мыши
        POINT pt;
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        ClientToScreen(hwnd, &pt);

        // Отображаем контекстное меню
        TrackPopupMenu(hContextMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
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
            case VK_DELETE:
            {
                auto it = std::find(shapes.begin(), shapes.end(), selectedShape);
                if (it != shapes.end()) {
                    delete* it;               // Удаляем объект из памяти
                    shapes.erase(it);          // Удаляем указатель из вектора
                }
                selectedShape = nullptr;       // Сбрасываем указатель
                break;
            }

            }
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;

    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        for (MyShapes::Shape* shape : shapes) {
            // Проверяем тип фигуры перед рисованием
            if ((dynamic_cast<MyShapes::Line*>(shape) && showLines) ||
                (dynamic_cast<MyShapes::Circle*>(shape) && showCircles) ||
                (dynamic_cast<MyShapes::Arc*>(shape) && showArcs) ||
                (dynamic_cast<MyShapes::Ring*>(shape) && showRings) ||
                (dynamic_cast<MyShapes::Polyline*>(shape) && showPolylines) ||
                (dynamic_cast<MyShapes::Polygon*>(shape) && showPolygons) ||
                (dynamic_cast<MyShapes::Triangle*>(shape) && showTriangles) ||
                (dynamic_cast<MyShapes::Parallelogram*>(shape) && showParallelograms)) {
                shape->draw(hdc);
            }
        }
        EndPaint(hwnd, &ps);
        break;

    case WM_DESTROY:
        for (MyShapes::Shape* shape : shapes) {
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
    std::vector<MyShapes::Shape*> shapes;
    shapes.push_back(new MyShapes::Circle(MyShapes::Point(200, 200), 100));
    shapes.push_back(new MyShapes::Line(MyShapes::Point(100, 100), MyShapes::Point(300, 300)));

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}
