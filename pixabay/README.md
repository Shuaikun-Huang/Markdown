# **前言**
因为公众号每次都需要图片才能群发，但人为一张张下载又很傻，所以想到用爬虫去爬取。本文旨在总结下一个简单的爬虫步骤，加深自己的印象，如有表达不准确的地方望指出，一起学习。本文爬虫使用python语言，基于cookie的爬取，所爬取的图片来自于[pixabay](https://pixabay.com/zh/)网站。  
# **爬虫步骤**
**1.目的**：  
首先需要明确爬虫的目标是什么？因为公众号规定最大上传5M内的图片，所以我的目标是范围内的最高分辨率的图片。  
**2.起始url**：  
接着要明确你爬取的起始链接，也就是你一开始请求的网页，这个网页应该具有你想要的东西。我选择的是pixabay的搜索页面```https://pixabay.com/zh/images/search/```， 它内容如下：
<center>
    <img src="https://raw.githubusercontent.com/Shuaikun-Huang/Markdown/master/pixabay/pixabay-index.png" />  
    <br>
    <div style="color:orange; border-bottom: 1px solid #d9d9d9;
    display: inline-block;
    color: #999;
    padding: 2px;">初始url</div>
</center> 

该页面的图片便是我们想要的东西。接着发现该页面存在**下一页**按钮，可以缓一口气了，还好不是[动态加载页面](https://baike.baidu.com/item/%E5%8A%A8%E6%80%81%E7%BD%91%E9%A1%B5)，点击下一页，观察url的变化，可以发现第二页的url结尾多了```?pagi=2```，多验证几个，可以总结出起始页面为```https://pixabay.com/zh/images/search/?pagi=1```，**更改最后的数字可以实现切换页数功能**。  

**3.分析页面信息**  
**右键检查**进入开发者模式，最上方需要关注element和network，其中element是请求url后response的正文内容，network是请求url后的加载过程，可以查看具体的请求头与响应头以及其他参数。  
<center>
    <img src="https://raw.githubusercontent.com/Shuaikun-Huang/Markdown/master/pixabay/pixabay-f12.png" />  
    <br>
    <div style="color:orange; border-bottom: 1px solid #d9d9d9;
    display: inline-block;
    color: #999;
    padding: 2px;">开发者模式</div>
</center>   

先选任意一张图片**右键检查**，如下：
<center>
    <img src="https://raw.githubusercontent.com/Shuaikun-Huang/Markdown/master/pixabay/pixabay-ele.png" />  
    <br>
    <div style="color:orange; border-bottom: 1px solid #d9d9d9;
    display: inline-block;
    color: #999;
    padding: 2px;">element</div>
</center>  

可以发现里面有很多链接，随便打开一个都是这张图片，只是size不同，很明显这些图是缩略图，如果对图片分辨率不做要求，我们可以直接爬取这些链接并下载，即可。但是我的目标是高画质的图片，所以需要继续寻找高画质图片的链接才行。  
点开一张图，重复上面过程，发现这张图也只是分辨率高了一点而已，不满足要求。接着目光移到免费下载按钮上，可以发现其中1920*1279是满足我们要求的最高分辨率图像：
<center>
    <img src="https://raw.githubusercontent.com/Shuaikun-Huang/Markdown/master/pixabay/pixabay-freedownload.png" />  
    <br>
    <div style="color:orange; border-bottom: 1px solid #d9d9d9;
    display: inline-block;
    color: #999;
    padding: 2px;">免费下载按钮</div>
</center>  

鼠标放“查看”上，**右键检查**：
<center>
    <img src="https://raw.githubusercontent.com/Shuaikun-Huang/Markdown/master/pixabay/pixabay-href.png" />  
    <br>
    <div style="color:orange; border-bottom: 1px solid #d9d9d9;
    display: inline-block;
    color: #999;
    padding: 2px;">1920高清图链接</div>
</center>  

其中href字段后面就是目标地址，将href接在首页地址后，就形成了一个完整的高清图片请求链接，```https://pixabay.com/zh/images/download/landscape-4508444_1920.jpg```。  
我们要做的就是用代码去请求这个链接，并下载服务器返回给我们的响应体中的content。  
**4.整理思路**：  
>（1）请求起始base_url，获取img字段中的图片名称name；  
>（2）将获取的名称组合成一个完整链接download_url```https://pixabay.com/zh/images/download/imgname.jpg```；  
>（3）请求上述连接，并将响应的content进行下载；  
>（4）修改起始url中的页数，并重复2-3步  

# **代码实现**
**1.请求起始url，并获取图片名称**：
```
import re  #正则表达式
import requests #请求url
from lxml import etree #解析起始url正文内容

#get请求起始链接，并返回正文内容
html = requests.get(base_url).text 
#解析正文内容
tree = etree.HTML(html)
#提取缩略图链接，我选择的是data-lazy字段
img_name = tree.xpath('//img/@data-lazy')
#提取图片名字构成高分辨率图片链接
img_url=[]
#月-日-时-分，正则表达式匹配，只保留图片名
reg = re.compile(r"/[0-9]{2}/[0-9]{2}/[0-9]{2}/[0-9]{2}/(.*)__")
#这是一个页面的所有图片链接
for i in img_name:
    name=re.findall(reg, i) #提取名字，name是一个列表
    #组合高分辨率图片链接,如果不加1920，可获取原始图片，一般在5M以上
    img_url.append(download_url+name[0]+"_1920.jpg") 
```  

**2.请求下载url，并将响应内容进行下载**：
```
#保存路径
img_path = img_root+name+".jpg"  
#请求高清图片
img = requests.get(download_url)
#将响应的内容进行下载，下载其实就是写操作
with open(img_path, 'wb') as f:
    f.write(img.content)  
```
**3.循环请求**：  
将上面的两个模块放入for循环中，即可。  

# **问题分析**
上面基本上就是一个简单爬虫的结构，既不用模拟登录，也不用模拟动态请求。虽然结构正确，但是按照上面代码进行爬取，是爬不到任何东西，因为你会被阻挡在下面的界面，而你**下载的图片内容只是它的文本内容**：
<center>
    <img src="https://raw.githubusercontent.com/Shuaikun-Huang/Markdown/master/pixabay/pixabay-robot.png" />  
    <br>
    <div style="color:orange; border-bottom: 1px solid #d9d9d9;
    display: inline-block;
    color: #999;
    padding: 2px;">robot</div>
</center>  

其实你可以模拟这个过程，将download_url从**无痕模式**和**正常模式**下打开，你会发现在正常模式下可以看图片，而在无痕模式下出现的是上图界面。出现这个差别，原因出自于cookie。  

# **cookie与session**
**1.概念**：  
由于HTTP协议是**无状态**协议，所以服务端需要记录用户的状态时，就需要用某种机制来识具体的用户，这个机制就是**session**，其中session是保存在**服务端**的，有一个唯一标识。举个栗子：
> 当你进行购物车下单时，由于HTTP协议无状态，其并不知道是哪个用户操作的，所以服务端要为特定的用户创建了特定的session来标识这个用户，并且跟踪用户，这样才知道购物车里面有什么。  

**那服务端又如何识别特定的客户呢**？答案就是cookie。每次HTTP请求的时候，**客户端**都会发送相应的cookie信息到服务端。实际上大多数的应用都是用cookie来实现session跟踪的，第一次创建session的时候，服务端会在HTTP协议中告诉客户端，需要在cookie里面记录一个session ID，**以后每次请求把这个会话ID发送到服务器**，就能确定是哪个客户端。  

**2.作用**：
如上所说，cookie可以用来让服务端“认识”客户端。除此之外，cookie还可以用来跳过登录环节，再举个栗子：  
> 设想你第一次登录一个网站，输入账号密码后，进入它的主页，此时你关掉页面并重新访问一次，你会发现这次不用输入账号密码，直接进入了主页。  

你一定经历过上述场景，原因就是，浏览器将这个登录信息写到cookie里，并存储在了本地磁盘。下次访问网页时，网页脚本就可以读取这个信息，并自动帮你把用户名和密码填写了。这也是cookie名称的由来，给用户的一点甜头。  

**3.小结**：
（1）session是**服务端**保存的一个数据结构，用来跟踪用户的状态，这个数据可以保存在集群、数据库、文件中；  
（2）cookie是**客户端**保存用户信息的一种机制，用来记录用户的一些信息，也是实现session的一种方式。  

# **总结**
**1.怎么获取cookie**：  
从正常模式下请求download_url（提前打开开发者模式），查看network下的请求过程：
<center>
    <img src="https://raw.githubusercontent.com/Shuaikun-Huang/Markdown/master/pixabay/pixabay-network.png" />  
    <br>
    <div style="color:orange; border-bottom: 1px solid #d9d9d9;
    display: inline-block;
    color: #999;
    padding: 2px;">请求过程</div>
</center>  

上图左边从上到下就是请求的顺序，第一个是请求的download_url，第二个是返回的图片内容。上图右边的response headers是响应体，里面有返回内容的一些信息，如content-length等，当content-length（正文长度）为0，说明没有请求到数据。requests headers是请求体，是请求页面需要发给服务器端的信息，其中就包含**cookie**，以及爬虫经常用到的[user-agent](https://baike.baidu.com/item/%E7%94%A8%E6%88%B7%E4%BB%A3%E7%90%86)。  

大致的请求过程如下：浏览器提交携带cookie的请求体 -> 服务端接收后通过cookie确定sessionid -> 返回响应体 -> 通过响应体中给的location字段重新请求 -> 服务端返回图片内容 -> 浏览器接收后显示
<center>
    <img src="https://raw.githubusercontent.com/Shuaikun-Huang/Markdown/master/pixabay/pixabay-response.png" />  
    <br>
    <div style="color:orange; border-bottom: 1px solid #d9d9d9;
    display: inline-block;
    color: #999;
    padding: 2px;">响应体</div>
</center>  

**2.怎么携带cookie请求**：  
```
#yourcookie = {"xxx":"sdsaf"}
html = requests.get(download_url, cookies = yourcookie)
```

**3.不足与解决方向**：  
cookie请求还是存在局限性：
- 同一cookie多次访问服务器时，服务器会进行重新验证，判断是否是爬虫访问。这个很好识别，一般爬虫对服务器的访问呈线性关系。针对此，可以通过睡眠时间变化来模拟人为访问，但当睡眠时间过长又会使得爬虫效率太低。
- 使用cookie爬虫，就不能通过修改user-agent或ip代理来模拟不同人的访问，因为cookie是具有唯一性的。
因此本文代码就出现弊端，抓取十几次就需要人为进行验证，并重新抓取。但就目前爬取的图片数量已经够公众号使用，所以有机会再解决吧。解决方向-可以模拟人为点击认证吧。完整代码可以访问[github](https://github.com/Shuaikun-Huang/pixabay)获取。  

最后展示下爬取的效果：
<center>
    <img src="https://raw.githubusercontent.com/Shuaikun-Huang/Markdown/master/pixabay/pixabay-result.png" />  
    <br>
    <div style="color:orange; border-bottom: 1px solid #d9d9d9;
    display: inline-block;
    color: #999;
    padding: 2px;">成果</div>
</center>