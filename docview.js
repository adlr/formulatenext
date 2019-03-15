class Point {
  constructor(x, y) {
    this.x = x + 0;
    this.y = y + 0;
  }
  equals(that) {
    return this.x == that.x && this.y == that.y;
  }
}

class PagePoint {
  constructor(page, x, y) {
    this.page = page;
    this.x = x + 0;
    this.y = y + 0;
  }
}

class Size {
  constructor(width, height) {
    this.width = width + 0;
    this.height = height + 0;
  }
  equals(that) {
    return this.width == that.width && this.height == that.height;
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
  toString() {
    return ['[', this.position.x, ', ',
            this.position.y, ', ',
            this.size.width, ', ',
            this.size.height, ']'].join('');
  }
  equals(that) {
    return this.position.equals(that.position) &&
      this.size.equals(that.size);
  }
  intersects(that) {
    const right = this.position.x + this.size.width;
    const bot = this.position.y + this.size.height;
    const that_right = that.position.x + that.size.width;
    const that_bot = that.position.y + that.size.height;
    return this.position.x < that_right &&
      right > that.position.x &&
      this.position.y < that_bot &&
      bot > that.position.y;
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

class PageCacheEntry {
  constructor() {
    this.pagerect = null;
    this.image = null;
    this.next = null;
    this.scaleFactor = 1;
  }
  hasImage() {
    return this.image ? true : false;
  }
  hasExactMatch(rect, scale) {
    return rect.equals(this.pagerect) && this.scaleFactor == scale;
  }
  acceptNext() {
    this.pagerect = this.next.pagerect;
    this.image = this.next.image;
    this.scaleFactor = this.next.scaleFactor;
  }
  load(page, rect, scale, done) {
    if (this.next) {
      if (this.next.hasExactMatch(rect, scale)) {
        console.log('already requesting');
        // TODO: call 'done' here?
        return;
      }
      console.log('cancelling previous load');
      this.next.image.src = '';
      this.next = null;
    }

    let next = new PageCacheEntry;
    next.image = new Image;
    let self = this;
    next.image.onload = function() {
      if ('naturalHeight' in this) {
        if (this.naturalHeight + this.naturalWidth === 0) {
          this.onerror();
          return;
        }
      } else if (this.width + this.height == 0) {
        this.onerror();
        return;
      }
      // At this point, there's no error.
      self.acceptNext();
      done(true);
    };
    next.image.onerror = function() {
      self.next = null;
      done(false);
    };
    next.image.src = '/loadPageImage?page=' + page;
    this.next = next;
  }
}

class PageCache {
  constructor() {
    this.entries = {};
  }
  entryForPage(page) {
    const key = page + '';
    if (this.entries.hasOwnProperty(key)) {
      return this.entries[key];
    }
    return null;
  }
  hasImageForPage(page) {
    return this.entryForPage(page) ? true : false;
  }
  hasImageForRect(page, pagerect) {
    if (!hasImageForPage(page)) {
      console.log("ERROR! called hasImageForRect when no page entry exists");
      return false;
    }
    return pagerect.equals(entryForPage(page).pagerect);
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
      if (!border.intersects(dirtyrect)) {
        continue;
      }
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
