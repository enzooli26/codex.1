<template>
  <div class="panel">
    <div ref="chartRef" style="width:100%;height:340px"></div>
  </div>
</template>

<script setup>
import { ref, watch, onMounted, onUnmounted } from 'vue'
import * as echarts from 'echarts'

const props = defineProps({ ranking: { type: Array, default: () => [] } })
const chartRef = ref(null)
let chart = null

function render() {
  if (!chart) return
  const names = props.ranking.map(v => (v.name || '').slice(0, 8))
  const values = props.ranking.map(v => v.revenue || 0)
  const maxVal = Math.max(10, ...values) * 1.2

  chart.setOption({
    title: { text: '各电站累计营收排行', left: 'center', textStyle: { fontSize: 15, fontWeight: 600, color: '#344158' } },
    tooltip: { trigger: 'axis', formatter: (p) => `${p[0].name}<br/>¥${p[0].value.toFixed(2)}` },
    grid: { top: 50, bottom: 40, left: 50, right: 20 },
    xAxis: { type: 'category', data: names, axisLabel: { color: '#8896ab', interval: 0, rotate: names.length > 4 ? 30 : 0 }, axisLine: { lineStyle: { color: '#e5eaf2' } } },
    yAxis: { type: 'value', max: maxVal, axisLabel: { color: '#8896ab', formatter: '¥{value}' }, splitLine: { lineStyle: { color: '#f0f3f8' } } },
    series: [{
      type: 'bar', data: values, barWidth: '50%',
      itemStyle: { color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
        { offset: 0, color: '#5D84E8' }, { offset: 1, color: '#3B5FC0' }
      ]), borderRadius: [6, 6, 0, 0] }
    }]
  }, true)
}

onMounted(() => {
  chart = echarts.init(chartRef.value)
  render()
  window.addEventListener('resize', () => chart?.resize())
})
onUnmounted(() => { chart?.dispose() })
watch(() => props.ranking, render, { deep: true })
</script>
