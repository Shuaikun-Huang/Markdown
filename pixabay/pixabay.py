import re
import sys
import time
import random
import requests
from lxml import etree

class pixabay:
    def __init__(self, page):
        #保存图片的名字
        self.img_name = []  
        #起始url
        self.base_url = "https://pixabay.com/zh/images/search/?pagi=%d"%page
        #图片下载的url
        self.download_url = "https://pixabay.com/zh/images/download/"
        #保存路径文件夹
        self.img_root = "./picture/"
        #请求头
        self.header = {
            "User-Agent":"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/77.0.3865.90 Safari/537.36"
        }
        #基于cookie的页面访问
        self.cookie = {"__cfduid":"d41fbfd893484183d737541da257e23371569142867",
                      "lang":"zh",
                      "_ga":"GA1.2.1338763897.1569142871",
                      "is_human":"1",
                      "csrftoken":"yKwjwswipLV6ah0cT8fZSTCq9uIbLCY5awRS6XUyfMwdltWDMJE0Ua4CrkTWVOe1",
                      "sessionid":".eJxVjMsOwiAUBf-FtWnKq6T-DLnAJUVKMTziwvjvYhdGt-fMzJNo6G3TvWLRG9SNXIlBaQ36RXABq1rQ2dkrEMwYJiUFMTOxGqc8uZCGtdmcY8DhPXKJ6Mb6kzRgIx5uvPeSb2jb1FvY62R7bTmd4BRO9ICEOheNCcL-9f5i4dOhXPGVSUpeb5TdQJ0:1iD1qV:TmaIMPT8FgjBO45NgnhPAxK4YK4",
                      "_gid":"GA1.2.1397513266.1569641870"
                      }

    def LoadUserAgents(self, uafile):
        #加载本地user-agent
        uas = []
        with open(uafile, 'rb') as uaf:   
            for ua in uaf.readlines():
                if ua:
                    uas.append(ua.strip()[1:-1 - 1])
        random.shuffle(uas)    #随机打乱列表
        return uas

    def geturl(self):
        html = requests.get(self.base_url, headers=self.header).text
        #提取低分辨率图片链接
        tree = etree.HTML(html)
        img_name = tree.xpath('//img/@data-lazy')
        #提取图片名字构成高分辨率图片链接
        img_url=[]
        #月-日-时-分，正则表达式匹配，只保留图片名
        reg = re.compile(r"/[0-9]{2}/[0-9]{2}/[0-9]{2}/[0-9]{2}/(.*)__")
        for i in img_name:
            name=re.findall(reg, i) #提取名字
            self.img_name.append(name) #存储名字
            img_url.append(self.download_url+name[0]+"_1920.jpg") #组合高分辨率图片链接
        return img_url
    
    def imgdownload(self, url, name):
        #保存路径
        img_path = self.img_root+name+".jpg"  
        #请求高清图片
        download_headers={
            "User-Agent":random.choice(self.LoadUserAgents("./user_agents.txt"))
        }
        print(download_headers["User-Agent"])
        img = requests.get(url, headers=download_headers, cookies = self.cookie)
        if "Content-Length" in img.headers:  #访问太多次，会被当做robot
            with open(img_path, 'wb') as f:
                f.write(img.content)  #下载
            return True
        else:
            return False

if __name__ == '__main__':
    for i in range(16, 160):
        pix = pixabay(i)
        try:
            url_list = pix.geturl()
        except Exception:
            print("geturl(page= %d) failed"%i)
            continue
        j = -1 #记录当前是第几张图
        for url in url_list:
            j += 1
            if(len(url) == 0):
                continue
            else:
                try:
                    flag = pix.imgdownload(url, pix.img_name[j][0])
                    if(flag == False): #如果被识别出是robot，就退出程序
                        print("you are a robot")
                        sys.exit(0)
                except Exception:
                    print("download %s failed"%pix.img_name[j][0])
            time.sleep(2)
        print("--------------第%d页下载完成---------------"%i)