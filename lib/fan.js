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

    // Ping the server every 15 seconds to keep the connection alive.
    let pingpong = setInterval(() => {
      if (ws.readyState) {
        ws.send('ping')
      }
    }, 15000)

    ws.on('open', () => {
      this.reconnecting = false
      this.emit('ready')
      this.emit('websocket-status', `Connected to ws://${this.opts.host}:${this.opts.port}`)
    })

    ws.on('close', (e) => {
      clearInterval(pingpong)
      this.reconnect(e)
    })

    ws.on('error', (e) => {
      clearInterval(pingpong)
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
