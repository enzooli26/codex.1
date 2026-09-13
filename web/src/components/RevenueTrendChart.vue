<template>
  <div class="panel">
    <div ref="chartRef" style="width:100%;height:340px"></div>
  </div>
</template>

<script setup>
import { ref, watch, onMounted, onUnmounted } from 'vue'
import * as echarts from 'echarts'

const props = defineProps({ trend: { type: Array, default: () => [] } })
const chartRef = ref(null)
let chart = null

function render() {
  if (!chart || !chartRef.value) return
  const dates = props.trend.map(v => v.date?.slice(5) || '')
  const amounts = props.trend.map(v => v.amount || 0)
  const maxVal = Math.max(10, ...amounts) * 1.2

  chart.setOption({
    title: { text: '近 7 天营收趋势', left: 'center', textStyle: { fontSize: 15, fontWeight: 600, color: '#344158' } },
    tooltip: { trigger: 'axis', formatter: (p) => `${p[0].axisValue}<br/>¥${p[0].value.toFixed(2)}` },
    grid: { top: 50, bottom: 30, left: 50, right: 20 },
    xAxis: { type: 'category', data: dates, axisLabel: { color: '#8896ab' }, axisLine: { lineStyle: { color: '#e5eaf2' } } },
    yAxis: { type: 'value', max: maxVal, axisLabel: { color: '#8896ab', formatter: '¥{value}' }, splitLine: { lineStyle: { color: '#f0f3f8' } } },
    series: [{
      type: 'line', data: amounts, smooth: true, symbol: 'circle', symbolSize: 8,
      lineStyle: { color: '#4878e8', width: 3 },
      itemStyle: { color: '#4878e8' },
      areaStyle: { color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
        { offset: 0, color: 'rgba(72,120,232,0.25)' }, { offset: 1, color: 'rgba(72,120,232,0.02)' }
      ]) }
    }]
  }, true)
}

onMounted(() => {
  chart = echarts.init(chartRef.value)
  render()
  window.addEventListener('resize', () => chart?.resize())
})
onUnmounted(() => { chart?.dispose() })
watch(() => props.trend, render, { deep: true })
</script>
