class Point {
  constructor(x, y) {
    this.x = x;
    this.y = y;
  }
}

class PagePoint {
  constructor(page, x, y) {
    this.page = page;
    this.x = x;
    this.y = y;
  }
}

class Size {
  constructor(width, height) {
    this.width = width;
    this.height = height;
  }
  scaled(factor) {
    return new Size(this.width * factor, this.height * factor);
  }
}

class Rect {
  constructor(position, size) {
    this.position = position;
    this.size = size;
  }
  inset(amount) {
    return new Rect(new Point(this.position.x + amount,
			      this.position.y + amount),
		    new Size(this.size.width - 2 * amount,
			     this.size.height - 2 * amount));
  }
  ctxRect(ctx) {
    ctx.rect(this.position.x,
	     this.position.y,
	     this.size.width,
	     this.size.height);
  }
}

class DocView {
  constructor(doc, sizechanged) {
    this.doc = doc;
    this.margin = 20;  // pixels
    this.zoomLevel = 1;
    this.sizechanged = sizechanged;  // callback
    this.size = new Size(0, 0);
    this.computePageRectsAndSize();
  }
  computePageRectsAndSize() {
    this.pageRects = [];
    // first, compute max page width
    let width = 0;
    for (let i = 0; i < doc.pages.length; i++) {
      let pageWidth = doc.pages[i].size.width * this.zoomLevel;
      width = Math.max(width, pageWidth);
    }
    // compute page rects
    let top = this.margin;
    for (let i = 0; i < doc.pages.length; i++) {
      let pageSize = doc.pages[i].size.scaled(this.zoomLevel);
      this.pageRects.push(new Rect(new Point(
	this.margin + (width - pageSize.width) / 2.0,
	top), pageSize));
      top += pageSize.height + this.margin;
    }
    // top is now the very bottom
    this.size = new Size(width + 2 * this.margin, top);
    this.sizechanged();
  }
  pixelToPagePoint(point) {
    
  }
  PagePointToPixel(pagepoint) {

  }
  zoom(factor) {
    this.zoomLevel *= factor;
    this.computePageRectsAndSize();
  }
  zoomabs(amount) {
    if (this.zoomLevel == amount)
      return;
    this.zoomLevel = amount;
    this.computePageRectsAndSize();
  }
  draw(ctx, dirtyrect) {
    // TODO: only draw in dirtyrect
    ctx.strokeStyle = "#000";
    ctx.lineWidth = 1;
    for (let i = 0; i < this.pageRects.length; i++) {
      const pageRect = this.pageRects[i]
      const border = pageRect.inset(-0.5);
      ctx.save();
      ctx.strokeRect(border.position.x,
		     border.position.y,
		     border.size.width,
		     border.size.height);
      ctx.beginPath();
      pageRect.ctxRect(ctx);
      ctx.fillStyle = "#ffffff";
      ctx.fill();
      ctx.fillStyle = "#000000";
      ctx.clip();
      ctx.translate(pageRect.position.x,
		    pageRect.position.y);
      ctx.scale(this.zoomLevel, this.zoomLevel);
      this.doc.draw(ctx, i);
      ctx.restore();
    }
  }
}
