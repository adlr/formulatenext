# formulatenext

Plan is for a simple client-side web page. It should operate natively
on PDF files, rather than having a custom file format.

Core code will be written in C++, compiled w/ Emscripten, with glue
code to the browser written in JavaScript.

## Software dependencies

* pdfium/skia - rendering PDF for web-based GUI, editing
* opencv - signature import/image cleanup
* (later) harfbuzz - rendering Unicode text to PDF

## Current features

* Ability to open and view a PDF
* Arbitrary zoom in/out
* Thumbnail view
* Save document
* Select (multiple) pages in thumbnail view and drag to reorder
* Draw/freehand (and edit when reopened)
* Add/edit text in PDF documents
* Append PDF to document

## Known issues

* Added text must be single-line
* Rotated pages don't seem to work well at all
* It's hard to add text on top of existing text (you end up editing existing text)
* Sometimes there's an error when appending a PDF
* Off-by-one pixel errors in the render cache can lead to artifacts (saved doc not impacted, luckily)

## Immediately Planned Features (in order I plan to work on them)

* Change added text to use FreeText Annotations rather than PDF text
  objects. Goal is to support multi-line text boxes.
* Move line drawing annotations to Ink(?) Annotations rather than
  using PDF path objects.
* Disallow editing anything except annotations

## Other TODOs in no particular order

* Rotate pages
* Delete pages
* Drag a PDF document in, or drag pages between document windows to copy pages
* Drop images into page (and edit when reopened)
* Better text handling (with Harfbuzz)
* Rich text
* Drop PDF into page (as Form XObject)
