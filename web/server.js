import tls from 'node:tls'
import { WebSocketServer } from 'ws'
import { encode, decode, request } from './server/framecodec.js'

const TLS_HOST = process.env.TLS_HOST || '127.0.0.1'
const TLS_PORT = parseInt(process.env.TLS_PORT || '9527', 10)
const WS_PORT = parseInt(process.env.WS_PORT || '8080', 10)
const ADMIN_USER = process.env.ADMIN_USER || 'admin'
const ADMIN_PASS = process.env.ADMIN_PASS || '123456'

let tlsSocket = null
let tlsBuffer = Buffer.alloc(0)
let loggedIn = false
let backendStatus = 'disconnected'
const wsClients = new Set()

function broadcastStatus() {
  const msg = JSON.stringify({ type: 'bridge.status', data: { backend: backendStatus } })
  for (const client of wsClients) {
    if (client.readyState === 1) client.send(msg)
  }
}

function connectTLS() {
  if (tlsSocket && tlsSocket.destroyed === false) return

  console.log(`[bridge] connecting to TLS server ${TLS_HOST}:${TLS_PORT}`)
  tlsBuffer = Buffer.alloc(0)
  loggedIn = false
  backendStatus = 'connecting'
  broadcastStatus()

  tlsSocket = tls.connect({
    host: TLS_HOST,
    port: TLS_PORT,
    rejectUnauthorized: false
  }, () => {
    console.log('[bridge] TLS connected, sending auth...')
    backendStatus = 'connected'
    broadcastStatus()
    sendToServer(request('auth.admin', { username: ADMIN_USER, password: ADMIN_PASS }))
  })

  tlsSocket.on('data', (chunk) => {
    tlsBuffer = Buffer.concat([tlsBuffer, chunk])
    const { messages, remaining } = decode(tlsBuffer)
    tlsBuffer = remaining
    for (const msg of messages) handleServerMessage(msg)
  })

  tlsSocket.on('error', (err) => {
    console.error('[bridge] TLS error:', err.message)
  })

  tlsSocket.on('close', () => {
    console.log('[bridge] TLS disconnected, reconnecting in 3s...')
    tlsSocket = null
    loggedIn = false
    backendStatus = 'disconnected'
    broadcastStatus()
    setTimeout(connectTLS, 3000)
  })
}

function sendToServer(msg) {
  if (tlsSocket && tlsSocket.destroyed === false) {
    tlsSocket.write(encode(msg))
  }
}

function broadcastToClients(msg) {
  const data = JSON.stringify(msg)
  for (const client of wsClients) {
    if (client.readyState === 1) client.send(data)
  }
}

function handleServerMessage(msg) {
  if (msg.type === 'auth.admin.result') {
    if (msg.code === 0) {
      console.log('[bridge] admin login successful')
      loggedIn = true
      backendStatus = 'ready'
      broadcastStatus()
      startPolling()
    } else {
      console.error('[bridge] admin login failed:', msg.message)
      backendStatus = 'auth_failed'
      broadcastStatus()
    }
    return
  }

  broadcastToClients(msg)
}

let summaryTimer = null
let chargerTimer = null
let ordersTimer = null

function startPolling() {
  clearInterval(summaryTimer)
  clearInterval(chargerTimer)
  clearInterval(ordersTimer)

  summaryTimer = setInterval(() => {
    if (loggedIn) sendToServer(request('admin.summary'))
  }, 5000)

  chargerTimer = setInterval(() => {
    if (loggedIn) sendToServer(request('admin.chargers'))
  }, 2000)

  ordersTimer = setInterval(() => {
    if (loggedIn) sendToServer(request('admin.orders'))
  }, 5000)

  sendToServer(request('admin.summary'))
  sendToServer(request('admin.chargers'))
  sendToServer(request('admin.orders'))
}

const wss = new WebSocketServer({ port: WS_PORT }, () => {
  console.log(`[bridge] WebSocket server listening on ws://127.0.0.1:${WS_PORT}`)
})

wss.on('connection', (ws) => {
  wsClients.add(ws)
  console.log(`[bridge] browser connected, total: ${wsClients.size}`)

  ws.send(JSON.stringify({ type: 'bridge.status', data: { backend: backendStatus } }))

  if (loggedIn) {
    sendToServer(request('admin.summary'))
    sendToServer(request('admin.chargers'))
    sendToServer(request('admin.orders'))
  }

  ws.on('message', (raw) => {
    try {
      const msg = JSON.parse(raw.toString())
      if (msg.type === 'ack') return
      if (msg.type === 'request' && msg.payload) {
        sendToServer(request(msg.payload.type, msg.payload.payload || {}))
      }
    } catch {}
  })

  ws.on('close', () => {
    wsClients.delete(ws)
    console.log(`[bridge] browser disconnected, total: ${wsClients.size}`)
  })
})

connectTLS()
