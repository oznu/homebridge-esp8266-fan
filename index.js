'use strict'

const WebSocket = require('@oznu/ws-connect')

var Service, Characteristic

module.exports = function (homebridge) {
  Service = homebridge.hap.Service
  Characteristic = homebridge.hap.Characteristic
  homebridge.registerAccessory('homebridge-esp8266-fan', 'ESP8266 Fan', FanAccessory)
}

class FanAccessory {
  constructor (log, config) {
    this.log = log
    this.config = config
    this.service = new Service.Fan(this.config.name)

    this.status = {
      power: false,
      speed: 0
    }

    this.fan = new WebSocket(`ws://${this.config.host}:${this.config.port}`, {
      options: {
        handshakeTimeout: 2000
      }
    })
    this.fan.on('websocket-status', this.log)

    this.fan.on('json', (status) => {
      if ('power' in status) {
        this.status.power = status.power
        this.service.getCharacteristic(Characteristic.On).updateValue(status.power)
      }
      if ('speed' in status) {
        this.status.speed = status.speed
        this.service.getCharacteristic(Characteristic.RotationSpeed).updateValue(status.speed)
      }
    })

    this.fan.on('error', (error) => {
      this.log(error)
    })
  }

  getServices () {
    const informationService = new Service.AccessoryInformation()
      .setCharacteristic(Characteristic.Manufacturer, 'oznu')
      .setCharacteristic(Characteristic.Model, 'esp8266-fan')
      .setCharacteristic(Characteristic.SerialNumber, 'oznu-esp8266-fan')

    this.service
      .getCharacteristic(Characteristic.On)
      .on('get', this.getOn.bind(this))
      .on('set', this.setOn.bind(this))

    this.service
      .getCharacteristic(Characteristic.RotationSpeed)
      .on('get', this.getRotationSpeed.bind(this))
      .on('set', this.setRotationSpeed.bind(this))

    this.service
      .getCharacteristic(Characteristic.Name)
      .on('get', this.getName.bind(this))

    return [informationService, this.service]
  }

  getName (callback) {
    callback(null, this.config.name)
  }

  getOn (callback) {
    callback(null, this.status.power)
  }

  setOn (value, callback) {
    if (this.status.power !== value) {
      this.log(`Setting fan power state - ${value ? 'ON' : 'OFF'}`)
      this.fan.sendJson({power: value, speed: this.status.speed || 25})
    }
    callback(null)
  }

  getRotationSpeed (callback) {
    callback(null, this.status.speed)
  }

  setRotationSpeed (value, callback) {
    this.fan.sendJson({speed: value})
    callback(null)
  }

}
