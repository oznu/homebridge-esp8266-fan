'use strict'

const ESP8266Fan = require('./lib/fan')

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

    this.fan = new ESP8266Fan(this.config)
    this.fan.on('websocket-status', this.log)

    this.fan.on('fan-status', (status) => {
      this.status.power = status.power
      this.status.speed = status.speed
    })
  }

  getName (callback) {
    callback(null, this.config.name)
  }

  getOn (callback) {
    callback(null, this.status.power)
  }

  setOn (value, callback) {
    this.log(`Setting fan power state - ${value}`)
    this.fan.send({power: value})
    callback(null)
  }

  getRotationSpeed (callback) {
    callback(null, this.status.speed)
  }

  setRotationSpeed (value, callback) {
    this.fan.send({speed: value})
    callback(null)
  }

  getServices () {
    var informationService = new Service.AccessoryInformation()
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
}
