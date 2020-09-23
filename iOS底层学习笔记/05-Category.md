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

**原因：** 通过结构看到值打印了分类test里面的实现，而分类Eat和本来里面的实现没有打印。只打印分类Test的实现不打印分类Eat的实现，是因为编译顺序的问题，分类Test在分类Eat之后编译，最终运行时Test的方法列表在一个大数组前面；不打印本类是因为本类的方法在运行时就被安排到了方法大数组的最后面。只要找到一个对应的方法名，即调用。

#### 源码分析分类数据加载过程

* objc-os.mm 这是运行时的入口
	* _objc_init 运行时初始化
	* map_images 所有模块或镜像
	* map_images_nolock 解锁所有模块或镜像
![](resource/05/12.png)
![](resource/05/13.png)

* objc-runtime-new.mm
	* _read_images
	* remethodizeClass
	* attachCategories
	* attachLists
	* realloc、memmove、 memcpy
![](resource/05/14.png)
![](resource/05/15.png)
![](resource/05/16.png)
![](resource/05/17.png)

```
//cls  : 类
//cats : 该类对应的所有的分类，是个分类列表
static void attachCategories(Class cls, category_list *cats, bool flush_caches){
    if (!cats) return;
    if (PrintReplacedMethods) printReplacements(cls, cats);
    bool isMeta = cls->isMetaClass();
    // fixme rearrange to remove these intermediate allocations
    // 方法列表 二维数组 [[method_t, method_t], [method_t, method_t]]
    method_list_t **mlists = (method_list_t **)
        malloc(cats->count * sizeof(*mlists));
    // 属性列表
    property_list_t **proplists = (property_list_t **)
        malloc(cats->count * sizeof(*proplists));
    // 协议列表
    protocol_list_t **protolists = (protocol_list_t **)
        malloc(cats->count * sizeof(*protolists));

    // Count backwards through cats to get newest categories first
    int mcount = 0;
    int propcount = 0;
    int protocount = 0;
    int i = cats->count;
    bool fromBundle = NO;
    while (i--) { // i-- 从后向前遍历
        auto& entry = cats->list[i];// 取出分类

        //取出分类方法列表
        method_list_t *mlist = entry.cat->methodsForMeta(isMeta);
        if (mlist) {
            //把该分类的方法列表放入前面定义的大数组mlists中。
            //mcount++ 是从0开始
            mlists[mcount++] = mlist;
            fromBundle |= entry.hi->isBundle();
        }
        property_list_t *proplist = 
            entry.cat->propertiesForMeta(isMeta, entry.hi);
        if (proplist) {
            proplists[propcount++] = proplist;
        }
        protocol_list_t *protolist = entry.cat->protocols;
        if (protolist) {
            protolists[protocount++] = protolist;
        }
    }
    // rw 是objc_class结构体中用来拿到类对象方法列表的一个数据结构
    // 这是取出类对象中的数据
    auto rw = cls->data();

    prepareMethodLists(cls, mlists, mcount, NO, fromBundle);
    //将所有分类的对象方法附加到类对象的方法列表中
    rw->methods.attachLists(mlists, mcount);
    free(mlists);
    if (flush_caches  &&  mcount > 0) flushCaches(cls);
    
    ////将所有分类的属性方法附加到类对象的属性列表中
    rw->properties.attachLists(proplists, propcount);
    free(proplists);
    
    //将所有分类的协议附加到类对象的协议
    rw->protocols.attachLists(protolists, protocount);
    free(protolists);
}
```

```
void attachLists(List* const * addedLists, uint32_t addedCount) {
        if (addedCount == 0) return;

        if (hasArray()) {
            // many lists -> many lists
            uint32_t oldCount = array()->count;
            //数量增加分类的数量
            uint32_t newCount = oldCount + addedCount;
            //realloc重新初始化内存大小newCount
            setArray((array_t *)realloc(array(), array_t::byteSize(newCount)));
            array()->count = newCount;
            
            //array()已经扩大内存了
            //memmove是把array()->lists本类方法列表在大数组中往后移动addedCount个位置。
            //addedCount就是本类拥有的分类个数，也就是本类所有分类的方法列表个数
            memmove(array()->lists + addedCount, array()->lists, 
                    oldCount * sizeof(array()->lists[0]));
            //把分类的方法列表addedLists移动到array()->lists数组的最前面
            //本类自己的方法列表放在了最后面
            //所以假如本类和分类都含有同名的方法，在运行时遍历到一个大数组中的第一个分类方法列表时
            //如果找到该方法就直接调用了。
            //而分类间的同名方法调用顺序则就完全依赖分类在大数组中的前后顺序了
            //分类的前后顺序也就是编译顺序，XCode能控制。工程 -> BUild Phases -> Compile Source
            //最后编译的最终会放到大数组的最前面
            memcpy(array()->lists, addedLists, 
                   addedCount * sizeof(array()->lists[0]));
        }
        else if (!list  &&  addedCount == 1) {
            // 0 lists -> 1 list
            list = addedLists[0];
        } 
        else {
            // 1 list -> many lists
            List* oldList = list;
            uint32_t oldCount = oldList ? 1 : 0;
            uint32_t newCount = oldCount + addedCount;
            setArray((array_t *)malloc(array_t::byteSize(newCount)));
            array()->count = newCount;
            if (oldList) array()->lists[addedCount] = oldList;
            memcpy(array()->lists, addedLists, 
                   addedCount * sizeof(array()->lists[0]));
        }
    }
```
形象些如下：
![](resource/05/18.png)

* 原理如下：
    * 程序运行时通过Runtime加载某个类的所有Category数据
    * 把所有Category的方法、属性、协议数据，合并到一个大数组中，后参与编译的Category数据，会在数组的前面。
    * 将合并后的分类数据（方法、属性、协议），插入到类原来数据的前面。

### Category和Extension
* Category
    * Category编译之后的底层结构是struct category_t，里面存储着分类的对象方法、类方法、属性、协议信息
    * 在程序运行的时候，runtime会将Category的数据，合并到类信息中（类对象、元类对象中）
*  Extension
    * Extension在编译的时候，它的数据就已经包含在类信息中
    * Category是在运行时，才会将数据合并到类信息中
    
### Load 方法    
#### load方法调用顺序
**前提**
* 示例中Son继承自Person，Person有两个分类Test和Eat；
* 每个类中都有一个类方法load
看下面打印：
![](resource/05/19.png)

**结果：** 在没有import的情况下、没有调用的情况下，程序跑起来变打印出了load方法。且有如下调用顺序：
* **先调用父类的load （没有继承关系的类之间按照编译顺序调用，先编译则先调用）**
* **再调用子类的load**
* **最后调用分类的load（分类之间按照编译顺序调用，无论是否有继承关系，先编译则先调用）**

**疑问** 
* 根本还没有调用load方法，为什么就打印出来了？
* 按照前面一节的知识来讲，当分类中存在与本类相同的方法时，调用的时候也会只调用分类的方法，为什么load方法无论分类还是本类都被调用了呢？ 

**解答**
* +load方法会在运行时加载类、分类时自动被调用；每个类、分类的+load，在程序运行过程中只调用一次，且调用顺序和编译顺序有关。
* 比如上节讲的run方法在分类和本类中和都有，但调用的确是分类中的run。
    * 是因为**[person run ]**这种形式的本质是runtime的发消息机制：**objc_msgSend**；
    * 涉及到消息发送，就会按照isa指针找类对象或者元类对象中的对象方法或类方法调用，如果某个类恰巧存在分类则该分类中的那个方法就被调用了；
    * 而load方法调用机制不同：load方法在运行时加载所有类时，会直接拿到每个类中的load方法地址，直接调用执行，没有涉及到消息发送机制，也就不会按照isa机制来查找方法。

#### load调用源码分析
源码解读：
* objc-os.mm
    * _objc_init 运行时加载类、分类
* objc-runtime-new.mm
    * load_images
    * prepare_load_methods 准备工作
        * schedule_class_load 定制规划类的顺序问题
        * objc-loadmethod.mm
            * add_class_to_loadable_list 添加类到数组中
            * add_category_to_loadable_list 添加分类到数组中
    * objc-loadmethod.mm
        * call_load_methods 依次调用数组中所有类的load方法
            * call_class_loads 调用类的load方法
            * call_category_loads  调用分类的load方法
            * (*load_method)(cls, SEL_load) 函数指针直接调用方法
             
下面分部看：

* _objc_init 运行时加载类、分类
![](resource/05/20.png)
* load_images
![](resource/05/27.png) 
* prepare_load_methods 准备工作
![](resource/05/28.png)
    * schedule_class_load 定制规划类的顺序问题
    ![](resource/05/26.png)
    * add_class_to_loadable_list 添加类到数组中
    ![](resource/05/24.png)
    * add_category_to_loadable_list 添加分类到数组中
    ![](resource/05/25.png) 
* call_load_methods 依次调用数组中所有类的load方法
![](resource/05/21.png)
    * call_class_loads 调用类的load方法
    ![](resource/05/22.png)
    * call_category_loads  调用分类的load方法
    ![](resource/05/23.png)
    * (*load_method)(cls, SEL_load) 函数指针直接调用方法
    ![](resource/05/29.png)
        
### initialize 方法          
#### 结论
* +initialize方法会在类第一次接收到消息时调用。
* 调用顺序
    * 在父类和子类都实现了initialize的前提下，先调用父类的+initialize，再调用子类的+initialize，否则调用了你也不知道。
    * 先初始化父类，再初始化子类，每个类只会初始化1次

* **+initialize方法会在类第一次接收到消息时调用**
![](resource/05/30.png)
* **先初始化父类，再初始化子类，每个类只会初始化1次**
![](resource/05/31.png)

#### initialize源码分析
**分析** 因为initialize方法的调用是消息发送机制，而是仅仅在一次接收消息时调用。那么消息机制，就会涉及到根据ISA找到类对象或元类对象，再接着找方法(也就是第一次接收的消息的那个方法，比如alloc)。那么重点就是找方法：class_getInstanceMethod
* objc-runtime-new.mm
    * class_getInstanceMethod 找方法
    * lookUpImpOrNil 查询方法列表
    * lookUpImpOrForward 实际查询函数地址
    * objc-initialize.mm 
        * _class_initialize 调用的初始化方法
        * callinitialize 实际调用
        * objc_msgSend(cls, SEL_initialize) 最终还是objc_msgSend
* objc-msg-arm64.s // arm64汇编文件
    * objc_msgSend // 我的乖乖，原来大名鼎鼎的objc_msgSend()方法是由汇编实现的

* **class_getInstanceMethod**
![](resource/05/33.png)

* **lookUpImpOrNil**
![](resource/05/34.png)

* **_class_initialize**
![](resource/05/35.png)

* **_class_initialize**
![](resource/05/36.png)

* **callinitialize**
![](resource/05/37.png)

* **callinitialize**
![](resource/05/38.png)

* **objc_msgSend**
![](resource/05/32.png)

**延伸** +initialize和+load的很大区别是，+initialize是通过objc_msgSend进行调用的，所以有以下特点：
* 如果子类没有实现+initialize，会调用父类的+initialize（所以父类的+initialize可能会被调用多次）
    * 比如，首先Person有一个分类，且本类和分类都实现了initialize方法
    * Son和Teacher都是Person的子类，并且两个子类都没有实现initialize方法，也没有分类
    * 那么Son第一次接收消息时，首先会检查父类Person是否已经初始化，如果没有则先调用一次父类的initialize方法，又因为Person和其分类都实现了，所有最终会调用Person分类中实现的initialize，此时打印一次。
    * 父类调用完成，该调用Son自己的initialize，但Son没有实现，则会找到父类的该方法，进行调用。又打印一次。
    * 接着Teacher首次接收到了消息，那么首先会检查父类Person是否已经初始化，很明显前面已经初始化过了，不再调用。紧接着调用Teacher的initialize，但Teacher也没实现，那么就会查找父类的该方法，然后又被调用一次。第三次打印。
* 如果分类实现了+initialize，就覆盖类本身的+initialize调用
    * 这种情况很好理解，这就是ISA的寻找方法的机制。分类和本类有同名方法，只调用分类的方法。
### 分类添加成员变量
* 默认情况下，因为分类底层结构的限制(没有ivars成员变量数组)，不能添加成员变量到分类中。但可以通过关联对象来间接实现。
* 分类中直接写成员变量会报错
* 分类中添加属性，系统默认只会生成setter和getter的声明，不会生成对应的实现和成员变量。
* 关联对象并不是存储在被关联对象本身内存中
* 关联对象存储在全局的统一的一个AssociationsManager中* 设置关联对象为nil，就相当于是移除关联对象

#### setter实现
* 添加关联对象

```
void objc_setAssociatedObject(id object, const void * key,id value, objc_AssociationPolicy policy)
```
![](resource/05/39.png)

##### objc_AssociationPolicy 策略
![](resource/05/41.png)

##### key的常见用法
* static void *MyKey = &MyKey;

```
objc_setAssociatedObject(obj, MyKey, value, OBJC_ASSOCIATION_RETAIN_NONATOMIC)objc_getAssociatedObject(obj, MyKey)
```

* static char MyKey;

```
objc_setAssociatedObject(obj, &MyKey, value, OBJC_ASSOCIATION_RETAIN_NONATOMIC)objc_getAssociatedObject(obj, &MyKey)
```

* 使用属性名作为key
```
objc_setAssociatedObject(obj, @"property", value, OBJC_ASSOCIATION_RETAIN_NONATOMIC);objc_getAssociatedObject(obj, @"property");
```

* 使用get方法的@selecor作为key

```
objc_setAssociatedObject(obj, @selector(getter), value, OBJC_ASSOCIATION_RETAIN_NONATOMIC)objc_getAssociatedObject(obj, @selector(getter))
```

**备注**
* 前两个用法有瑕疵，因为是全局变量当多个实例都调用分类中的属性时数据就会错乱。
* 后面两种是常用的，但最后一种更便利。
#### getter实现
* 获取关联对象

```
id objc_getAssociatedObject(id object, const void * key)
```
![](resource/05/40.png)

#### 关联对象的原理
* 实现关联对象技术的核心对象有
    * AssociationsManager
    * AssociationsHashMap
    * ObjectAssociationMap
    * ObjcAssociation

* objc4源码解读：objc-references.mm
![](resource/05/42.png)
![](resource/05/43.png)
![](resource/05/44.png) ![](resource/05/45.png)
![](resource/05/46.png)
![](resource/05/47.png)

**备注**
* 移除所有的关联对象

```
void objc_removeAssociatedObjects(id object)
```