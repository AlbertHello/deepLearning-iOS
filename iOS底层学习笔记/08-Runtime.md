2020年10月5日  书店 天气不错，晴冷

## runtime 
首先介绍下C语言的有关位运算的知识
### 按位与 按位或 
* 在arm64架构之前，isa就是一个普通的指针，存储着Class、Meta-Class对象的内存地址
* 从arm64架构开始，对isa进行了优化，变成了一个共用体（union）结构，还使用**位域**来存储更多的信息
![](resource/08/01.png)

C语言位运算
* 与运算& 
	* 两个同时为1，结果为1，否则为0
	* 比如 0000 1000 & 0000 1001 = 0000 1000
* 或运算|
	* 只要一个为1，其值为1。
	* 比如 0000 1000 | 0000 1001 = 0000 1001

* 取反运算~
	* 按位取反，0->1,1->0
	* ~0000 1000 = 1111 0111

* & 常用来取值
	* 比如有二进制0001 1010，我想知道第四个二进制位是0还是1，那么我可以让这个数&上一个掩码0000 1000
	* 那么0001 1010 & 0000 1000 = 0000 1000，如果得到的数第四位为1那么远来得那个数第四位也一定为1，如果为0那么原来的那个数的第四位也是0。

* | 常用来赋值
	* 比如有二进制数0001 1010，我想让他第一位变为1，那么可以让它|上一个掩码0000 0001
	* 那么0001 1010 | 0000 0001 = 0001 1011 
	* 这样既能保证不改变其他位的值，也能把第一位置位1.
	* 如果把第二位置0怎么办呢？先对掩码取反，再&
	* 也就是先取反 ~0000 0010 = 1111 1101
	* 然后再与 0001 1010 & 1111 1101 = 0001 1000

#### 对象的属性用& | 来赋值取值
加入此时我创建一个对象，这个对象有三个布尔类型的属性。正常情况下我们应该是这么创建的：
![](resource/08/02.png)

这样最终会占用三个字节，其实我们可以用一个字节就可以完成。

我们这样做：
![](resource/08/03.png)

我们只定义一个char类型的成员变量，它只占用一个字节，但我们怎么把三个属性用它表示呢？

这样做：
![](resource/08/04.png)

我们给成员变量用二进制赋值。三个属性的值分别用最低位的三个二进制位表示

那么我们就不需要属性了，我们改为如下的接口：
![](resource/08/05.png)

前面已经温习了C语言的知识，那下面就是取值和赋值的实现：


* tall 属性
![](resource/08/06.png)

* rich 属性
![](resource/08/07.png)

* handsome属性
![](resource/08/08.png)

* 其中的宏是：
![](resource/08/09.png)

* 左移几位就是乘上2的几次方
* 右移几位就是除上2的几次方
* 前面讲取第一位就与上0000 0001 这就是 1<<0
* 前面讲取第二位就与上0000 0010 这就是 1<<1
* 前面讲取第三位就与上0000 0100 这就是 1<<2

至此三个属性只占用一个字节的demo就完成了：
![](resource/08/10.png)

#### 对象的属性用 & | 来赋值取值优化
我们对成员变量进行优化如下：
![](resource/08/11.png)

成员变量改为一个结构体变量_tallRichHandsome，结构体有三个成员，但每个成员规定了只占一位，且由上而下占用由低到高的二进制位，也就是说tall 占用第一个二进制位，rich占用第二个二进制位，handsome占用第三个二进制位。由于结构体的内存对齐原则，最终_tallRichHandsome还是只占用一个字节。

随之取值赋值改变如下：
![](resource/08/12.png)

赋值操作就按照常规操作正常赋值就能完事儿:
![](resource/08/13.png)


#### 对象的属性用 & | 来赋值取值再优化-共用体
来看下面：
![](resource/08/14.png)

这个是共用体结构，来存储我们的四个属性的值。最终也只需要1个字节，起作用的其实就只有bits成员。这个结构已经和isa_t共用体非常相似了：
![](resource/08/01.png)

demo中的共用体也等价于下面的：
![](resource/08/15.png)

也进一步等价于下面的：
![](resource/08/16.png)

 而且当使用共用体存储属性值时，其setter和getter方法的实现和只用一个char类型成员时的实现一模一样：
 ![](resource/08/06.png)
 
 至此位域大概就是这样了，下面仔细看下isa_t共用体
 
#### 位运算补充
开发中我们经常看到有如下用法:
![](resource/08/29.png)

设置多个或运算后，内部怎么取值的？
我们也自己定义：
![](resource/08/24.png)

传入值之后再&就能能得到它的值了。
![](resource/08/23.png)
 
### isa_t共用体详解
  ![](resource/08/18.png)
  
  * 前面说过了在64位之后，isa指针指向的类对象或元类对象的地址需要做一次位运算才能拿到相应的地址
  * 64位之前isa指针指向的就直接是类对象地址或元类对象地址。
  ![](resource/08/01.png)
  
  从上图可以里看到占用33位的那个就是指向的类对象或元类对象地址，参与运算的掩码是：0xFFFFFFFF8：
![](resource/08/19.png)
    
 用计算器显示下二进制如下：
 ![](resource/08/20.png)
 
 中间的33位都是1，最低位的三位都是0，那么通过 & 运算得到的地址最后三位一定都是0，也就是我们得到以下结论：
![](resource/08/21.png)

类对象或元类对象的地址最后三位永远都是0：
![](resource/08/22.png)

#### isa_t成员变量详解
下面挨个来看看isa_t每个成员的含义:
* nonpointer
	* 0，代表普通的指针，存储着Class、Meta-Class对象的内存地址
	* 1，代表优化过，使用位域存储更多的信息
* has_assoc
	* 是否有设置过关联对象，如果没有，释放时会更快
* has_cxx_dtor
	* 是否有C++的析构函数（.cxx_destruct），如果没有，释放时会更快，看如下源码：
![](resource/08/27.png)* shiftcls
	* 存储着Class、Meta-Class对象的内存地址信息* magic
	* 用于在调试时分辨对象是否未完成初始化* weakly_referenced
	* 是否有被弱引用指向过，如果没有，释放时会更快
* deallocating
	* 对象是否正在释放* extra_rc
	* 里面存储的值是引用计数器减1
* has_sidetable_rc
	* 引用计数器是否过大无法存储在isa中。
	* 如果为1，那么引用计数会存储在一个叫SideTable的类的属性中

#### Class的结构
![](resource/08/26.png)
![](resource/08/28.png)
![](resource/08/37.png)

##### class_rw_t
* class_rw_t里面的methods、properties、protocols是二维数组，是可读可写的，包含了类的初始内容、分类的内容
![](resource/08/38.png)


* method_t是对方法\函数的封装
![](resource/08/39.png)

* IMP代表函数的具体实现
* SEL代表方法\函数名，一般叫做选择器，底层结构跟char *类似
	* 可以通过@selector()和sel_registerName()获得
	* 可以通过sel_getName()和NSStringFromSelector()转成字符串
	* 不同类中相同名字的方法，所对应的方法选择器是相同的
![](resource/08/30.png)

* types包含了函数返回值、参数编码的字符串
* iOS中提供了一个叫做@encode的指令，可以将具体的类型表示成字符串编码![](resource/08/42.png)
![](resource/08/43.png)  ![](resource/08/44.png)


![](resource/08/31.png)

其中i代表返回值是int类型，@代表第一个参数是id类型，:代表第二个参数是sel类型，i代表第三个参数是int类型，f代表第四个参数是float类型。

24代表参数总共占用24个字节，0代表从第0个字节开始是id类型的参数，8代表从第八个字节开始是sel类型的参数，16代表从第16个字节开始是int类型的参数，20代表从第20个字节开始是float类型的参数。


##### class_ro_t
* class_ro_t里面的baseMethodList、baseProtocols、ivars、baseProperties是一维数组，是只读的，包含了类的初始内容
![](resource/08/45.png)

##### cache_t 方法缓存
* Class内部结构中有个方法缓存（cache_t），用**散列表（哈希表）**来缓存曾经调用过的方法，可以提高方法的查找速度
![](resource/08/46.png)

散列表原理：
* 散列表也是哈希表，查找效率比数组高，是因为用空间换取了时间
* 通过传入的一个key，经过某种算法得到一个索引index，该key对应的值组成结构体bucket_t就存在获取到的索引index下。
* 当在某个index下存储时，可能其他的index并没有值，所以有部分空间是浪费的。
* 当通过key查找时，也是通过key利用某个算法再次获取到index，然后根据index直接取值，速度自然比数组快。
* 当两个或多个key通过某该算法得到了同一个index时，也就是产生了哈希碰撞，再会根据某个算法比如让得出的index-1或index+1或者其他什么算法，让index一个key一一对应就行。

**注意：**
* 实际上哈希冲突的解决办法一般有开放定址、再哈希、链地址等
* 开放定址：
    * 按照一定规则向其他地址探测，直到遇到空桶
* 再哈希：
    * 设计多个哈希函数
* 链地址：
    * 比如通过链表把同一index下的元素串起来。比如Java JDK1.8中就使用单链表和红黑树解决哈希冲突

![](resource/08/52.png)

---

iOS 这里 bucket_t 的原理：
* 当拿传入key时，会通过&上mask，得到index索引
![](resource/08/33.png)
* 当多个key通过&是哪个mask得到了同一个index时，iOS这里的处理方法是让index-1。
![](resource/08/35.png)
* 当散列表容量满了，会清空之前存储的，再扩容，因为mask变了，所以有必要清空之前的。
![](resource/08/36.png)

* &上mask得到的值最大也不会超过mask
![](resource/08/32.png)

下面看下实际的都没：
* 只调用了init方法之后：
![](resource/08/47.png)
容量是mask+1=4，occupied=1，表明只缓存了一个。

* 调用personTest 之后
![](resource/08/48.png)

我们恰巧能看到bucket数组的第一个元素是就是刚才调用过的，如果这为null，也是正常的，因为可能缓存的index位置可能不是0而已。

* 当调用多个方法之后，打散列表：
![](resource/08/49.png)

可以看到有的为null有的是空，所以印证了上面所说的散列表的某些位置是空的。

可以看下具体的通过key获取imp指针的实现:
![](resource/08/50.png)
![](resource/08/51.png)

可以看到传入的sel会通过&上mask得到index，再通过判断根据index取出来的selector是否等于传入的selector,只有相等的时候才会返回对应的imp。 如果不满足则下一个循环，index-1了。


### 消息发送
详细看下消息发送的执行流程：[obj instanceMethod] 大概转化成了如下形式：
 objc_msgSend(obj,instanceMethod)，其中obj称为消息接收者，instanceMethod是函数地址IMP。
* 1 首先如果接收者obj如果是nil，则消息发送就会停止，没有任何反应
* 2 obj不为nil，则就会根据obj的isa找到obj的类对象，然后在cache中国查找方法
* 3 cache找到了即调用，结束查找。
* 4 cache找不到则会在class_rw_t中查找方法，找到了即调用，并将方法缓存到类对象的cache中
* 5 class_rw_t中没有找到，则会在类对象中利用superClass指针找到父类
* 6 然后在父类的cache中查找，找到了即调用，结束查找。
* 7 否则在父类的class_rw_t中查找，找到了即调用，并将方法缓存到类对象的cache中。
* 8 如果父类的class_rw_t中还还查找不到，那就重复第5步。
* 9 如果父类到头儿了还没找到方法就回进入动态解析阶段

如下图示：
![](resource/08/53.png)

### 动态解析
消息发送阶段如果找不到方法，就回进入到动态解析阶段，看下源码先：
![](resource/08/54.png)

* 动态解析阶段有个if判断，第一个参数是应该系统传入的resolver，这个是YES。重点是第二个参数triedResolver，这个参数在本函数开头，定义为：`bool triedResolver = NO;`
* 所以当第一次进入动态解析时，能进入到 if 执行代码。
* 动态解析完后，triedResolver被设置为YES，所以对该方法的动态解析就执行一次。
* 解析完后就回到函数开头处重新走消息发送的流程，查chche、class_rw_t等，找到就调用。找不到就进入下一阶段消息转发。

那么动态解析上层应该这么做呢？
* 首先，实现两个方法
```
// 处理类方法找不到
+ (BOOL)resolveClassMethod:(SEL)sel{
    if (sel == @selector(testClassMethord)) {
        
        // 方案1 添加 C 函数
        // 第一个参数是object_getClass(self)
        // 第二个参数是就是传入的sel
        // 第三个参数是将要动态添加的方法的IMP指针
        // 第四个是将要动态添加的方法的参数类型、返回值类型等描述
//        class_addMethod(object_getClass(self),
//                        sel,
//                        (IMP)c_other, // 如果是C函数，那么函数名就是函数地址了。
//                        "v16@0:8");
        
        // 方案2 添加 OC 函数
        // 第一个参数是object_getClass(self)
        // 第二个参数是就是传入的sel
        // 第三个参数是将要动态添加的方法的IMP指针
        // 第四个是将要动态添加的方法的参数类型、返回值类型等描述
        
        Method method = class_getInstanceMethod(self, @selector(other));
        IMP imp = method_getImplementation(method); // 获取到OC函数的函数地址

        class_addMethod(object_getClass(self),
                        sel,
                        imp,
                        method_getTypeEncoding(method));
        return YES; // 返回YES代表有动态添加方法
    }
    return [super resolveClassMethod:sel];
}
```
```
// 处理实例方法找不到
+ (BOOL)resolveInstanceMethod:(SEL)sel{
    if (sel == @selector(testInstanceMethord)) {
        
        // 方案1 添加 C 函数
        // 第一个参数是 self 代表实例对象
        // 第二个参数是就是传入的sel
        // 第三个参数是将要动态添加的方法的IMP指针
        // 第四个是将要动态添加的方法的参数类型、返回值类型等描述
//        class_addMethod(self,
//                        sel,
//                        (IMP)c_other,
//                        "v16@0:8");
        
        // 方案2 添加 OC 函数
        // 第一个参数是 self 代表实例对象
        // 第二个参数是就是传入的sel
        // 第三个参数是将要动态添加的方法的IMP指针
        // 第四个是将要动态添加的方法的参数类型、返回值类型等描述
        
        Method method = class_getInstanceMethod(self, @selector(other));
        IMP imp = method_getImplementation(method); // 获取到OC函数的函数地址

        class_addMethod(self,
                        sel,
                        imp,
                        method_getTypeEncoding(method));
        
        return YES; // 返回YES代表有动态添加方法
    }
    return [super resolveInstanceMethod:sel];
}
```
* 还有要动态添加的方法，也就是找不到某个方法时，临时让一个方法顶替的意思
```
// C 函数
void c_other(id self, SEL _cmd){
    NSLog(@"c_other - %@ - %@", self, NSStringFromSelector(_cmd));
}
// OC 函数
- (void)other{
    NSLog(@"%s", __func__);
}
```
* `+resolveClassMethod` 和 `+resolveInstanceMethod` 分别是类方法找不到和实例方法找不到时被系统自动调用的方法
* 通过method_getImplementation函数动态添加一个方法实现。
* 动态添加的这个方法可以是C函数也可以是OC的方法。
* 如此，便解决了方法找不到的问题。
图示：
![](resource/08/55.png)

**另外：**
* 上面代码中的`Method`其实在底层就是`struct method_t`
* `Method`的定义是`typedef struct objc_method *Method;`
* 而 `struct objc_method == struct method_t`

```
struct method_t {
    SEL sel;
    char *types;
    IMP imp;
};

// 获取其他方法
struct method_t *method = (struct method_t *)class_getInstanceMethod(self, @selector(other));
// 动态添加test方法的实现
class_addMethod(self, sel, method->imp, method->types);
```
### 消息转发
通过上面的源码可以看到如果动态解析阶段还没解决，那么就进入到消息转发阶段了。





