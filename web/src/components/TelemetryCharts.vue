<template>
  <div class="telemetry-charts">
    <div class="panel">
      <div ref="voltageRef" style="width:100%;height:300px"></div>
    </div>
    <div class="panel">
      <div ref="currentRef" style="width:100%;height:300px"></div>
    </div>
    <div class="panel">
      <div ref="powerRef" style="width:100%;height:300px"></div>
    </div>
  </div>
</template>

<script setup>
import { ref, watch, onMounted, onUnmounted } from 'vue'
import * as echarts from 'echarts'

const props = defineProps({ chargers: { type: Array, default: () => [] } })

const voltageRef = ref(null)
const currentRef = ref(null)
const powerRef = ref(null)
let voltageChart, currentChart, powerChart

const STATION_COLORS = ['#4878e8', '#e86448', '#48c864', '#c848c8']

function aggregateByStation() {
  const volMap = {}, curMap = {}, powMap = {}
  for (const c of props.chargers) {
    const st = c.station || ''
    if (!st) continue
    if (c.status !== 'CHARGING') continue
    const v = c.voltage || 0, cur = c.current || 0, p = c.livePower || 0
    if (v > 0 || cur > 0 || p > 0) {
      if (!volMap[st]) volMap[st] = { sum: 0, count: 0 }
      if (!curMap[st]) curMap[st] = { sum: 0, count: 0 }
      if (!powMap[st]) powMap[st] = { sum: 0, count: 0 }
      volMap[st].sum += v; volMap[st].count++
      curMap[st].sum += cur; curMap[st].count++
      powMap[st].sum += p; powMap[st].count++
    }
  }
  const stations = [...new Set([...Object.keys(volMap), ...Object.keys(curMap), ...Object.keys(powMap)])]
  const result = { stations, voltage: [], current: [], power: [] }
  for (const st of stations) {
    result.voltage.push(volMap[st] ? volMap[st].sum / volMap[st].count : 0)
    result.current.push(curMap[st] ? curMap[st].sum / curMap[st].count : 0)
    result.power.push(powMap[st] ? powMap[st].sum / powMap[st].count : 0)
  }
  return result
}

function makePieOption(title, data, unit, color) {
  return {
    title: { text: title, left: 'center', textStyle: { fontSize: 14, fontWeight: 600, color: '#344158' } },
    tooltip: { trigger: 'item', formatter: '{b}: {c} ' + unit + ' ({d}%)' },
    legend: { bottom: 0, textStyle: { color: '#596780', fontSize: 11 } },
    series: [{
      type: 'pie', radius: ['35%', '60%'], center: ['50%', '45%'],
      data: data.map((v, i) => ({
        value: Math.abs(v),
        name: agg.stations[i] || '',
        itemStyle: { color: STATION_COLORS[i % STATION_COLORS.length] }
      })),
      label: { show: true, formatter: '{b}\n{c}' + unit },
      itemStyle: { borderRadius: 4, borderColor: '#fff', borderWidth: 2 }
    }]
  }
}

const agg = { stations: [], voltage: [], current: [], power: [] }

function render() {
  const data = aggregateByStation()
  agg.stations = data.stations
  agg.voltage = data.voltage
  agg.current = data.current
  agg.power = data.power

  if (voltageChart) voltageChart.setOption(makePieOption('充电中电桩平均电压', data.voltage, 'V', STATION_COLORS), true)
  if (currentChart) currentChart.setOption(makePieOption('充电中电桩平均电流', data.current, 'A', STATION_COLORS), true)
  if (powerChart) powerChart.setOption(makePieOption('充电中电桩平均功率', data.power, 'kW', STATION_COLORS), true)
}

let resizeHandler
onMounted(() => {
  voltageChart = echarts.init(voltageRef.value)
  currentChart = echarts.init(currentRef.value)
  powerChart = echarts.init(powerRef.value)
  render()
  resizeHandler = () => { voltageChart?.resize(); currentChart?.resize(); powerChart?.resize() }
  window.addEventListener('resize', resizeHandler)
})
onUnmounted(() => {
  window.removeEventListener('resize', resizeHandler)
  voltageChart?.dispose(); currentChart?.dispose(); powerChart?.dispose()
})
watch(() => props.chargers, render, { deep: true })
</script>

<style scoped>
.telemetry-charts {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 12px;
}

@media (max-width: 768px) {
  .telemetry-charts {
    grid-template-columns: 1fr;
  }
}
</style>
