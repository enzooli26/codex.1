<template>
  <div class="panel">
    <div ref="chartRef" style="width:100%;height:340px"></div>
  </div>
</template>

<script setup>
import { ref, watch, onMounted, onUnmounted } from 'vue'
import * as echarts from 'echarts'

const props = defineProps({
  idle: { type: Number, default: 0 },
  charging: { type: Number, default: 0 },
  fault: { type: Number, default: 0 },
  total: { type: Number, default: 0 }
})

const chartRef = ref(null)
let chart = null

const COLORS = ['#53C59B', '#4B7BE5', '#E66B7B', '#B5C0D2']

function render() {
  if (!chart) return
  const other = Math.max(0, props.total - props.idle - props.charging - props.fault)
  const data = [
    { value: props.idle, name: '空闲' },
    { value: props.charging, name: '充电中' },
    { value: props.fault, name: '故障' },
    { value: other, name: '其他' }
  ].filter(d => d.value > 0)

  chart.setOption({
    title: { text: '电桩状态分布', left: 'center', textStyle: { fontSize: 15, fontWeight: 600, color: '#344158' } },
    tooltip: { trigger: 'item', formatter: '{b}: {c} ({d}%)' },
    legend: { bottom: 10, textStyle: { color: '#596780' } },
    series: [{
      type: 'pie', radius: ['42%', '65%'], center: ['50%', '48%'],
      data,
      label: { show: true, formatter: '{b}\n{c}' },
      itemStyle: { borderRadius: 6, borderColor: '#fff', borderWidth: 2 },
      emphasis: { itemStyle: { shadowBlur: 10, shadowOffsetX: 0, shadowColor: 'rgba(0,0,0,0.15)' } }
    }],
    color: COLORS
  }, true)
}

onMounted(() => {
  chart = echarts.init(chartRef.value)
  render()
  window.addEventListener('resize', () => chart?.resize())
})
onUnmounted(() => { chart?.dispose() })
watch([() => props.idle, () => props.charging, () => props.fault, () => props.total], render)
</script>
