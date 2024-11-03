struct Number {
    double value;
    Number() {
        self.value = 0;
        print("Created a number");
    }
    Number(double value) {
        self.value = value;
        print("Created a number");
    }
};

func test(Number number) {
    number.value = 1;
}

func main() { 
    var x = vector[shared[Number]](4);
    x[0] = shared[Number](3);
    test(x[0]);
    print(x[0].value);
    return 0;  
}