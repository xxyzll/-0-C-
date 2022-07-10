## 性能测试
### 内存大小4G/I7-11800H
![alt](/test/img.jpg)

### 网络结构
![alt](/test/network_struct.png)

### 注册机制减少代码冗余(项目独有)
&ensp; 在解析键值对、或者执行请求时，如果使用if判断的代码你可能会写为这样  
```c++
string a = key: val;      //代表key:val键值对
for(int i=0; i< a.size(); i++){
    int j=i;
    while(j< a.size()&& a[j] != ':')
        j++;
    if(s.substr(i, j-i) == "123"){
        // 做key==123时的逻辑
    }
    if(s.substr(i, j-i) == "234")
    ...
}
// 或者执行请求时
if(method == GET){
    // 做GET时的逻辑
}
if(method == POST){
    // 做POST时的逻辑
}
...
```
&ensp; 每一次添加新的键值对请求都需要去修改解析的代码，这样很容易出错，并且不美观，所以我们可以使用一个注册机制避免这一点
```c++
bool do_key_123(string arg){
    // 做key==123时的逻辑
}
bool do_key_234(string arg){
    // 做key==234时的逻辑
}

unordered_map<string, bool(*)(string)> key_funs = {
                                    {"123",do_key_123},
                                    {"234",do_key_234}}
// 在解析时
for(int i=0; i< a.size(); i++){
    int j=i;
    while(j< a.size()&& a[j] != ':')
        j++;
    string key = s.substr(i, j-i);
    if(key_funs.find(key)!= key_funs.end())
        key_funs[key](a);
}
// 如果你需要加入新的键值对解析，仅仅需要这样做
bool new_key_fun(string arg){
    ...
}
//插入即可，不用改变解解析代码
key_funs[new_key] = new_key_fun;    
```
