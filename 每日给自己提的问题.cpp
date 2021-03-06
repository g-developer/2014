===================================================================
2014/04/21
1.os.open();打开文件的时候，文件的数据被读入到内核中吗？ //对于系统底层对应所有语言实现

自己的理解： 理论上来说是不会在open的时候就预读一些数据进来，应该是采用lazy_read的方式，在真正调用read的时候才读取数据进入内核的buffer_cache中，这里就要做一些thrick的技巧比如说预读多一些的数据。
内核写磁盘的的次数基本不会因为write函数的调用而改变，因为它是以磁盘的单位block来写的，（现在一般系统的默认block是4K）

写数据的时候涉及到sync的策略。因为不同的sync策略所面对的应用场景不同，效率也不同。
1.sync(fd)  ----> 文件完整性同步。保证一个文件的数据完整性，不会因为意外断电而丢失数据。
2.fdatasync(fd) ---->数据完整性同步。fdatasync大致仅需要一次磁盘操作，而fsync需要两次磁盘操作。举例说明一下，假如文件内容改变了，但是文件尺寸并没有发生变化，那调用fdatasync仅仅是把文件内容数据flush到磁盘，而fsync不仅仅把文件内容flush刷入磁盘，还要把文件的last modified time也同步到磁盘文件系统。last modified time属于文件的元数据，一般情况下文件的元数据和文件内容数据在磁盘上不是连续存放的，写完内容数据再写元数据，必然涉及到磁盘的seek，而seek又是机械硬盘速度慢的根源。。。
在某些业务场景下，fdatasync和fsync的这点微小差别会导致应用程序性能的大幅差异。
3.sync_file_range()
这个接口是linux从2.6.17之后实现的，是linux独有的非标准接口。这个接口提供了比fdatasync更为精准的flush数据的能力。详细请参照man。
4.void sync(void);
强制"buffer cache"中的数据全部flush到磁盘，并且要遵循文件完整性同步。
上面4种方式介绍完毕，open()系统调用的打开文件的标志位，比如O_DSYNC诸如此类的标志，对flush数据的影响和上面几个接口作用类似。

2014/04/22
2.为了利用多核资源，系统中开启了过多的线程是否是好事？
显而易见，具体多少线程合适需要在压力测试下通过分析数据才能确定。


2014/04/25
3.C++类的内存布局怎么样的====做一个总结。

2014/05/05
4.什么是文件映射：mmap
暂时只知道它直接将文件映射到进程的虚拟地址空间中，而不需要从内核空间拷贝一份出来到用户空间中。
映射的概念就是 是物理磁盘与进程虚拟地址的对应关系，事实上数据还没在真正的内存中。
mmap()会返回一个指针ptr，它指向进程逻辑地址空间中的一个地址，这样以后，进程无需再调用read或write对文件进行读写，而只需要通过ptr就能够操作文件。但是ptr所指向的是一个逻辑地址，要操作其中的数据，必须通过MMU将逻辑地址转换成物理地址。
建立内存映射并没有实际拷贝数据，这时，MMU在地址映射表中是无法找到与ptr相对应的物理地址的，也就是MMU失败，将产生一个缺页中断，缺页中断的中断响应函数会在swap中寻找相对应的页面，如果找不到（也就是该文件从来没有被读入内存的情况），则会通过mmap()建立的映射关系。
为什么效率高：
而mmap()也是系统调用，如前所述，mmap()中没有进行数据拷贝，真正的数据拷贝是在缺页中断处理时进行的，由于mmap()将文件直接映射到用户空间，所以中断处理函数根据这个映射关系，直接将文件从硬盘拷贝到用户空间，只进行了 一次数据拷贝 。因此，内存映射的效率要比read/write效率高。

