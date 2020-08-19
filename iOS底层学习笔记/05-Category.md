## Category
### Category的底层结构
#### 底层结构
查看Objc源码，category定义在objc-runtime-new.h中。如下
![](resource/05/01.png)

#### 分类编译后的结构
创建如下分类：
![](resource/05/02.png)
每个分类的.h和.m如下：
Test分类：
![](resource/05/03.png)
![](resource/05/04.png)

Eat分类：
![](resource/05/05.png)
![](resource/05/06.png)


使用`xcrun -sdk iphoneos clang -arch arm64 -rewrite-objc Person+Eat.m`命令对该分类文件进行编译，得到一个传cpp文件，就是编译后的代码，其中可以看到有个_category_t结构体：
![](resource/05/07.png)

编译后eat分类中的对应代码生成如下：
* 实例方法列表
![](resource/05/08.png)
* 类方法列表
![](resource/05/09.png) 
* 属性列表
![](resource/05/10.png)
* 同样还有遵守的协议不再继续添加图片。

**总结** 所以分类在编译后，会生成每个分类对应的category的结构体，方法、属性等都存在结构体中
### Category的加载处理过程
#### 分类方法调用 
* 如果分类中实现了本类中的一个方法，在调用时是什么现象？
![](resource/05/11.png)

**原因：** 通过结构看到值打印了分类test里面的实现，而分类Eat和本来里面的实现没有打印。只打印分类Test的实现不打印分类Eat的实现，是因为编译顺序的问题，分类Test在分类Eat之后编译，最终运行时Test的方法列表在一个大数组前面；不打印本来是因为，本类的方法在运行时就被安排到了方法大数组的最后面。只要找到一个对应的方法名，即调用。
* 原理如下：
    * 程序运行时通过Runtime加载某个类的所有Category数据
    * 把所有Category的方法、属性、协议数据，合并到一个大数组中，后参与编译的Category数据，会在数组的前面。
    * 将合并后的分类数据（方法、属性、协议），插入到类原来数据的前面。

#### 源码分析分类数据加载过程

