// Mock TallyArbiter (listener side of v3.3.0) for testing the tally firmware
// without a real server. Same socket.io version and the same listener events;
// listener_reassign_object is refused without a login, as in 3.3.0.
//
//   npm install && node server.js [deviceId] [port] [device 1 name]
//
// deviceId: the tally's current TallyArbiter device id (Settings / log), so it
// lands on device 1. Commands on stdin:
//   pv <n> | pgm <n> | off <n>   tally on device n (1 or 2)
//   reassign <n>                 move every listener to device n
//   flash                        flash every listener
//   list                         listener clients as the server sees them

const http = require('http')
const readline = require('readline')
const { Server } = require('socket.io')

const port = Number(process.argv[3] || 4455)
const devices = [
  { id: process.argv[2] || 'mock-dev-1', name: process.argv[4] || 'MOCK 1', enabled: true, linkedBusses: [] },
  { id: 'mock-dev-2', name: 'MOCK 2', enabled: true, linkedBusses: [] },
]
const bus_options = [
  { id: 'e393251c', label: 'Preview', type: 'preview', color: '#3fe481', priority: 50, visible: true },
  { id: '334e4eda', label: 'Program', type: 'program', color: '#e43f5a', priority: 100, visible: true },
]
const active = {}          // deviceId -> Set of busIds
const listeners = []       // { id, socketId, deviceId, listenerType, inactive }

const log = (...a) => console.log(new Date().toISOString().slice(11, 19), ...a)
const dev = (n) => devices[Number(n) - 1]
const nameOf = (id) => (devices.find((d) => d.id === id) || { name: `?${id}` }).name

function getDeviceStates(deviceId) {
  return devices
    .filter((d) => (deviceId ? d.id === deviceId : true))
    .flatMap((d) => bus_options.map((b) => ({
      busId: b.id, deviceId: d.id, sources: active[d.id]?.has(b.id) ? ['mock-source'] : [],
    })))
}

const httpServer = http.createServer()
const io = new Server(httpServer, { allowEIO3: true })

io.on('connection', (socket) => {
  log(`socket ${socket.id} from ${socket.handshake.address}`)
  socket.emit('chat_enabled', true)

  socket.on('listenerclient_connect', (obj) => {
    if (typeof obj !== 'object' || obj === null) obj = JSON.parse(String(obj))
    let deviceId = obj.deviceId
    let oldDeviceId = null
    if (!devices.find((d) => d.id === deviceId)) {
      oldDeviceId = deviceId
      deviceId = devices[0].id
      socket.emit('error', 'Invalid Device Id specified. Reassigning to the first Device on the server.')
    }
    socket.join('device-' + deviceId)
    const l = { id: Math.random().toString(16).slice(2, 10), socketId: socket.id, deviceId,
                listenerType: obj.listenerType, inactive: false }
    listeners.push(l)
    log(`listener connected: ${obj.listenerType} on ${nameOf(deviceId)} (${deviceId})`)
    socket.emit('bus_options', bus_options)
    socket.emit('devices', devices)
    socket.emit('device_states', getDeviceStates(deviceId))
    if (oldDeviceId !== null) socket.emit('reassign', oldDeviceId, deviceId, l.id)
  })

  socket.on('devices', () => socket.emit('devices', devices))
  socket.on('bus_options', () => socket.emit('bus_options', bus_options))
  socket.on('device_states', (id) => socket.emit('device_states', getDeviceStates(id)))

  // 3.3.0: requireRole('producer') -- a listener has no login
  socket.on('listener_reassign_object', (o) => {
    log(`REFUSED listener_reassign_object ${JSON.stringify(o)} (needs producer login since 3.3.0)`)
    socket.emit('error', 'Access token required. Please login to use this feature.')
  })

  socket.on('disconnect', (reason) => {
    for (const l of listeners) if (l.socketId === socket.id) l.inactive = true
    log(`socket ${socket.id} disconnected (${reason})`)
  })
})

function setBus(n, type) {
  const d = dev(n)
  if (!d) return log('no such device')
  active[d.id] = new Set(type ? [bus_options.find((b) => b.type === type).id] : [])
  io.to('device-' + d.id).emit('device_states', getDeviceStates(d.id))
  log(`${d.name}: ${type || 'off'}`)
}

readline.createInterface({ input: process.stdin }).on('line', (line) => {
  const [cmd, arg] = line.trim().split(/\s+/)
  if (cmd === 'pv') setBus(arg, 'preview')
  else if (cmd === 'pgm') setBus(arg, 'program')
  else if (cmd === 'off') setBus(arg, null)
  else if (cmd === 'reassign') {
    const d = dev(arg)
    if (!d) return log('no such device')
    for (const l of listeners.filter((x) => !x.inactive && x.deviceId !== d.id)) {
      log(`reassign ${l.listenerType}: ${nameOf(l.deviceId)} -> ${d.name}`)
      io.to(l.socketId).emit('reassign', l.deviceId, d.id, l.id)
    }
  } else if (cmd === 'flash') {
    for (const l of listeners.filter((x) => !x.inactive)) io.to(l.socketId).emit('flash', l.id)
  } else if (cmd === 'list') {
    for (const l of listeners)
      log(`${l.inactive ? 'inactive' : 'ACTIVE  '} ${l.listenerType} on ${nameOf(l.deviceId)}`)
  } else if (cmd) log('commands: pv|pgm|off <n>, reassign <n>, flash, list')
})

httpServer.listen(port, () => log(`mock TallyArbiter 3.3 on port ${port}, device 1 = ${devices[0].id}`))
