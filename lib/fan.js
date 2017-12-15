'use strict'

const WebSocket = require('ws')
const EventEmitter = require('events')

module.exports = class ESP8266Fan extends EventEmitter {

  constructor (opts) {
    super()

    this.opts = opts
    this.autoReconnectInterval = 5 * 1000

    this.listen()
  }

  listen () {
    const ws = new WebSocket(`ws://${this.opts.host}:${this.opts.port}`)

    this.ws = ws

    // Ping the server every 10 seconds to make sure the server is alive
    let pong
    let ping = setInterval(() => {
      if (ws.readyState === 1) {
        ws.ping('ping', {failSilently: false})
      }
    }, 10000)

    // If we don't hear back from the server after 2 pings then reset the connection
    let pongTimeout = () => {
      this.emit('websocket-status', `Not responding...`)
      clearInterval(ping)
      this.reconnect()
    }

    ws.on('pong', () => {
      clearTimeout(pong)
      pong = setTimeout(pongTimeout, 21000)
    })

    ws.on('open', () => {
      this.reconnecting = false
      this.emit('ready')
      this.emit('websocket-status', `Connected to ws://${this.opts.host}:${this.opts.port}`)
      pong = setTimeout(pongTimeout, 21000)
    })

    ws.on('close', (e) => {
      clearInterval(ping)
      this.reconnect(e)
    })

    ws.on('error', (e) => {
      clearInterval(ping)
      this.reconnect(e)
    })

    ws.on('message', (data) => {
      try {
        let status = JSON.parse(data)
        this.emit('fan-status', status)
      } catch (e) {
        this.emit('websocket-status', `Failed to parse message from fan: ${data}`)
      }
    })
  }

  reconnect (e) {
    if (!this.reconnecting) {
      this.ws.removeAllListeners()
      this.ws.terminate()
      this.emit('websocket-status', `Disconnected - retry in ${this.autoReconnectInterval}ms`)
      this.reconnecting = true
      setTimeout(() => {
        this.emit('websocket-status', 'Reconnecting...')
        this.reconnecting = false
        this.listen(true)
      }, this.autoReconnectInterval)
    }
  }

  send (payload) {
    this.ws.send(JSON.stringify(payload))
  }

  state (state, speed) {
    this.send({
      power: Boolean(state),
      speed: parseInt(speed)
    })
  }

}
