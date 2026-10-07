#define RING_BUF_SIZE 512

typedef struct {
  SensorData_t data[RING_BUF_SIZE];
  volatile uint32_t head;
  volatile uint32_t tail;
} RingBuffer_t;

/*初始化环形缓冲区*/
void Ring_init(RingBuffer_t *rb) {
  rb->head = 0;
  rb->tail = 0;
}

/*判空*/
bool Ring_isEmpty(RingBuffer_t *rb) { return (rb->head == rb->tail); }

/*判满*/
bool Ring_isFull(RingBuffer_t *rb) {
  return (((rb->head + 1) & (RING_BUF_SIZE - 1)) == rb->tail);
}

/*写入一条数据 */
bool ring_push(RingBuffer_t *rb, const SensorData_t *item) {
  if (ring_is_full(rb))
    return false; // 缓冲区满，写入失败
  rb->data[rb->head] = *item;
  rb->head = (rb->head + 1) & (RING_BUF_SIZE - 1);
  return true;
}
/*读取一条数据*/
bool ring_pop(RingBuffer_t *rb, SensorData_t *item) {
  if (ring_is_empty(rb))
    return false;

  *item = rb->data[rb->tail];
  rb->tail = (rb->tail + 1) & (RING_BUF_SIZE - 1);
  return true;
}

/*获取当前数据缓冲区的数据量*/
uint32_t ring_count(RingBuffer_t *rb) {
  return (rb->head - rb->tail) & (RING_BUF_SIZE - 1);
}