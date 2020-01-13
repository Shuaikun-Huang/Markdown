#!/usr/bin/python

#coding:utf-8
import sys,os
length = os.getenv('CONTENT_LENGTH')
if length:
    postdata = sys.stdin.read(int(length))
    print "Content-type:text/html\n"
    print '<html>'
    print '<head>'
    print '<title>POST</title>'
    print '</head>'
    print '<body>'
    print '<b> POST data </b><br>'
    print '<ul>'
    print postdata
    print '\n'
    for data in postdata.split('&'):
        print  '<li>'+data+'</li>'
    print '</ul>'
    print '</body>'
    print '</html>'
else:
    print "Content-type:text/html\n"
    print 'no found'