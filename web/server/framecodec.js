export function encode(obj) {
  const body = Buffer.from(JSON.stringify(obj), 'utf-8')
  const header = Buffer.alloc(4)
  header.writeUInt32BE(body.length, 0)
  return Buffer.concat([header, body])
}

export function decode(buffer) {
  const messages = []
  const MAX_FRAME = 1024 * 1024
  let offset = 0
  while (offset + 4 <= buffer.length) {
    const length = buffer.readUInt32BE(offset)
    if (length === 0 || length > MAX_FRAME) {
      return { messages, remaining: Buffer.alloc(0) }
    }
    if (offset + 4 + length > buffer.length) break
    try {
      const msg = JSON.parse(buffer.toString('utf-8', offset + 4, offset + 4 + length))
      messages.push(msg)
    } catch {}
    offset += 4 + length
  }
  return { messages, remaining: offset < buffer.length ? Buffer.from(buffer.subarray(offset)) : Buffer.alloc(0) }
}

let _reqId = 0

export function request(type, payload = {}, requestId) {
  return {
    version: 1,
    type,
    requestId: requestId || `bridge-${++_reqId}`,
    timestamp: Date.now(),
    payload
  }
}

export function response(originalMsg, code, message, data = {}) {
  return {
    version: 1,
    type: originalMsg.type + '.result',
    requestId: originalMsg.requestId,
    timestamp: Date.now(),
    code,
    message,
    data
  }
}
