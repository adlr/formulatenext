# formulatenext

Plan is for a simple client-side web page. It should operate natively
on PDF files, rather than having a custom file format.

Core code will be written in C++, compiled w/ Emscripten, with glue
code to the browser written in JavaScript.

Software dependencies:

* pdfium - rendering PDF for web-based GUI, editing
* (later) harfbuzz/skia - rendering Unicode text to PDF

Planned features:

* Ability to open and view a PDF
* Arbitrary zoom in/out
* Thumbnail view

At this point I expect a few key C++ classes:

* PDFDocument - The Model, and the bridge to PDFium
* DocController - processes input on the main doc view
* DocView - view for a document
* ThumbnailController - processes input on the thumbnail view
* ThumbnailView - view for the thumbnails

Then the following features can be added:

* Rotate pages
* Save document
* Select (multiple) pages in thumbnail view and drag to reorder
* Drag a PDF document in, or drag pages between document windows to copy pages
* Drop text onto a page
* Re-open doc and edit dropped text
* Draw/freehand (and edit when reopened)
* Drop images into page (and edit when reopened)
* Better text handling (with Harfbuzz)
* Rich text
* Drop PDF into page (as Form XObject)
