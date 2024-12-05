#!/usr/bin/env python
#
#   This script executes a minimal CGI test function.  Output should be valid
#   HTML and a valid response to the user.  It is an example of a script
#   conforming to RFC 3875 Section: 5.  NPH Scripts.
#
#   Authors: Athula Balachandran <abalacha@cs.cmu.edu>
#            Charles Rang <rang@cs.cmu.edu>
#            Wolfgang Richter <wolf@cs.cmu.edu>

import cgi
import cgitb
import os
# 启用详细的错误回溯
cgitb.enable()

# 输出 HTTP 头部
print("HTTP/1.1 200 OK")
print("Content-Type: text/html\r\n")
print()

# 输出 HTML 页面
print("<html><head><title>Login Result</title></head><body>")
print("<h1>Login Information</h1>")
'''
# 获取表单提交的数据
form = cgi.FieldStorage()

# 获取提交的账号和密码
username = form.getvalue("username")
password = form.getvalue("password")

# 显示用户提交的信息
if username and password:
    print("<p>Username: {}</p>".format(username))
    print("<p>Password: {}</p>".format(password))
else:
    print("<p>Both username and password are required.</p>")
'''
for key, value in os.environ.items():
    print(f"<p>{key}: {value}</p>")
print("</body></html>")
