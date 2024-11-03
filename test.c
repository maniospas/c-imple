fn add(vector[double] x, vector[double] y) {
    if(x.size()!=y.size())
        throw std@runtime_error("Different vec sizes");
    var z = vector[double](x.size());
    for(int i=0;i<x.size();++i)
        z[i] = x[i]+y[i];
    return z;
}
fn main() {
    var x = vector[double]({1,2,3,4});
    var y = vector[double]({1,2,3,4});
    var z = add(x,y);
    print(z[2]);
}