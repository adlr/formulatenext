#!/usr/bin/python

import cherrypy
import ctypes

lib = ctypes.cdll.LoadLibrary("/home/adlr/Code/repo/pdfium/out/Debug/libpdfium_shared.so");
print lib



# class Root(object):
#     @cherrypy.expose
#     def index(self):
#         return "Hello World3!"

# if __name__ == '__main__':
#     cherrypy.quickstart(Root(), '/')

