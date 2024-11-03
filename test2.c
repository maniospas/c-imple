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
    fn getValue() {
        return self.value;
    }
    fn inc(double value) {
        self.value += value;
        return self.value;
    }
};

type Signed {
    exists[double] self.value;
    exists[double] self.getValue();
    exists[double] self.inc(1);
};

fn test(Signed a) {
    print(a.value);
}

fn main() {
    var a = shared[Number](1);
    a.value = -1;
    test(a);
    return 0;
}
