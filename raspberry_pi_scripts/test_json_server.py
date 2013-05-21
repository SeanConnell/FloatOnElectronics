#!/usr/bin/python
import cherrypy
import simplejson

class Root:
    exposed = True

    def GET(self):
        return "some content"

    def POST(self):
        print simplejson.loads(cherrypy.request.body.read()) 
        return "created"

if __name__ == "__main__":
   cherrypy.quickstart(Root(), config={'/': {'request.dispatch': cherrypy.dispatch.MethodDispatcher()}})

def start_server():
   cherrypy.quickstart(Root(), config={'/': {'request.dispatch': cherrypy.dispatch.MethodDispatcher()}})

