import { ref, onUnmounted } from 'vue'

export function useWebSocket() {
  const connected = ref(false)
  const backendStatus = ref('disconnected')
  const listeners = []
  let ws = null
  let reconnectTimer = null
  let heartbeatTimer = null
  let reconnectDelay = 1000
  const MAX_DELAY = 30000

  function getWsUrl() {
    const loc = window.location
    const protocol = loc.protocol === 'https:' ? 'wss:' : 'ws:'
    return `${protocol}//${loc.hostname}:8080`
  }

  function connect() {
    if (ws && (ws.readyState === WebSocket.CONNECTING || ws.readyState === WebSocket.OPEN)) return

    try {
      ws = new WebSocket(getWsUrl())
    } catch {
      scheduleReconnect()
      return
    }

    ws.onopen = () => {
      connected.value = true
      reconnectDelay = 1000
      heartbeatTimer = setInterval(() => {
        if (ws && ws.readyState === WebSocket.OPEN) {
          ws.send(JSON.stringify({ type: 'ack' }))
        }
      }, 30000)
    }

    ws.onmessage = (e) => {
      try {
        const msg = JSON.parse(e.data)
        if (msg.type === 'bridge.status') {
          backendStatus.value = msg.data.backend
          return
        }
        for (const fn of listeners) fn(msg)
      } catch {}
    }

    ws.onclose = () => {
      connected.value = false
      backendStatus.value = 'disconnected'
      clearInterval(heartbeatTimer)
      scheduleReconnect()
    }

    ws.onerror = () => {
      ws.close()
    }
  }

  function scheduleReconnect() {
    clearTimeout(reconnectTimer)
    reconnectTimer = setTimeout(() => {
      reconnectDelay = Math.min(reconnectDelay * 1.5, MAX_DELAY)
      connect()
    }, reconnectDelay)
  }

  function reconnect() {
    clearTimeout(reconnectTimer)
    clearInterval(heartbeatTimer)
    if (ws) {
      ws.onclose = null
      ws.close()
      ws = null
    }
    reconnectDelay = 1000
    connected.value = false
    backendStatus.value = 'disconnected'
    connect()
  }

  function onData(fn) {
    listeners.push(fn)
  }

  function send(msg) {
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify(msg))
    }
  }

  connect()

  onUnmounted(() => {
    clearInterval(heartbeatTimer)
    clearTimeout(reconnectTimer)
    if (ws) ws.close()
  })

  return { connected, backendStatus, onData, send, reconnect }
}
