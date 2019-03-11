# formulatenext

Plan is for a simple utility w/ the following functions:

* Define simple JSON format to store a PDF blob and overlay content
* Web-based GUI to edit overlay content
* command line tool to flatten overlay content to a PDF

Software dependencies:

* pdfium - rendering PDF for web-based GUI
* (pikepdf/qpdf) - PDF manipulation
* cherrypy - simple web server

Future work:
* Better text handling:
  * Bold/Italics
  * Choose from base PDF fonts
  * Arbitrary (embedded) fonts
  * High-ascii
  * Unicode
  * Emoji
  * Kerning
  * Ligatures
  * Line wrapping
* Drawing/freehand
* Import PDF/raster images
