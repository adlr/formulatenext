class Page {
  constructor(size) {
    this.size = size;
  }
}

class Doc {
  constructor(numpages) {
    this.pages = [];
    for (let i = 0; i < numpages; i++) {
      this.pages.push(new Page(new Size(612, 792)));
    }
  }
  draw(ctx, page) {
    ctx.beginPath();
    ctx.moveTo(0, 0);
    ctx.lineTo(10, 0);
    ctx.lineTo(10, 10);
    ctx.lineTo(0, 10);
    ctx.lineTo(0, 0);
    ctx.lineTo(10, 10);
    ctx.stroke();
    ctx.font = "20px Arial";
    ctx.fillText("Page " + page, 30, 30);
  }
};
