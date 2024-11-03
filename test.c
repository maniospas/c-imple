struct Point {
    double x;
    double y;
    Point(double x, double y) {
        self.x = x;
        self.y = y;
    }
};
fn add(Point a, Point b) {
    double x = a.x+b.x;
    double y = a.y+b.y;
    return Point(x,y);
}
fn main() {
    var a = Point(1, 2);
    var b = Point(1, 2);
    var c = add(a, b);
    print(c.x);
    print(c.y);
    return 0;
}