/* Compact validation map for the pinned MusyX PC asset decoder. Included after
 * PCReader/bytes; source bytes stay immutable until materialization completes.
 * One nibble per covered byte replaces a sorted {offset,value,width} record.
 * Sparse pages avoid allocating gaps for distant, individually bounded offsets.
 */
static PCField *fieldPage(PCReader *r, size_t offset) {
  size_t index = offset / 8192;
  PCField **link = &r->fields;
  if (r->last && r->last->index <= index)
    link = r->last->index == index ? &r->last : &r->last->next;
  while (*link && (*link)->index < index) link = &(*link)->next;
  if (!*link || (*link)->index != index) {
    PCField *page = salMalloc(sizeof(*page));
    if (!page) { fail(r, offset, "out of memory"); return NULL; }
    memset(page, 0, sizeof(*page));
    page->index = index; page->next = *link; *link = page;
  }
  r->last = *link;
  return *link;
}

static bool field(PCReader *r, size_t offset, u8 width) {
  if (r->failed) return false;
  for (unsigned i = 0; i < width; ++i) {
    PCField *p = fieldPage(r, offset + i);
    if (!p) return false;
    unsigned at = (offset + i) % 8192, shift = (at & 1) * 4;
    unsigned tag = i ? 8 : width, prior = (p->tags[at / 2] >> shift) & 15;
    if (prior && prior != tag)
      return fail(r, offset + i, "conflicting field interpretations");
    p->tags[at / 2] |= tag << shift;
  }
  return true;
}

static u16 word(PCReader *r, size_t offset) {
  const u8 *p = bytes(r, offset, 2);
  u16 value = p ? salPCReadBE16(p) : 0;
  field(r, offset, 2);
  return value;
}

static u32 dword(PCReader *r, size_t offset) {
  const u8 *p = bytes(r, offset, 4);
  u32 value = p ? salPCReadBE32(p) : 0;
  field(r, offset, 4);
  return value;
}

static bool raw(PCReader *r, size_t offset, size_t length) {
  if (!bytes(r, offset, length)) return false;
  for (size_t i = 0; i < length; ++i)
    if (!field(r, offset + i, 1)) return false;
  return true;
}

static void freeFields(PCReader *r) {
  while (r->fields) {
    PCField *next = r->fields->next;
    salFree(r->fields); r->fields = next;
  }
  r->last = NULL;
}

static void *materialize(PCReader *r) {
  if (r->failed) return NULL;
  u8 *output = salMalloc(r->extent ? r->extent : 1);
  if (!output) { fail(r, 0, "out of memory"); return NULL; }
  memcpy(output, r->span.data, r->extent);
  for (const PCField *p = r->fields; p; p = p->next) {
    size_t base = p->index * 8192;
    size_t length = r->extent - base;
    if (length > 8192) length = 8192;
    for (size_t i = 0; i < length; ++i) {
      unsigned tag = (p->tags[i / 2] >> ((i & 1) * 4)) & 15;
      const u8 *source = (const u8 *)r->span.data + base + i;
      if (tag == 2) {
        u16 value = salPCReadBE16(source); memcpy(output + base + i, &value, 2);
      } else if (tag == 4) {
        u32 value = salPCReadBE32(source); memcpy(output + base + i, &value, 4);
      }
    }
  }
  return output;
}
