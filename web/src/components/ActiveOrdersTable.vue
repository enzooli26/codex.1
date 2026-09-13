<template>
  <div class="panel">
    <div class="panel-title">充电中订单</div>
    <div class="table-wrapper">
      <table>
        <thead>
          <tr>
            <th>订单ID</th>
            <th>用户手机</th>
            <th>电站</th>
            <th>电桩</th>
            <th>已充电量(kWh)</th>
            <th>充电时长</th>
          </tr>
        </thead>
        <tbody>
          <tr v-if="orders.length === 0">
            <td colspan="6" style="color:#8896ab;padding:32px">暂无充电中订单</td>
          </tr>
          <tr v-for="o in orders" :key="o.id">
            <td>{{ o.id }}</td>
            <td>{{ o.phone }}</td>
            <td>{{ o.station }}</td>
            <td>{{ o.charger }}</td>
            <td>{{ (o.energy || 0).toFixed(2) }}</td>
            <td>{{ formatDuration(o.duration) }}</td>
          </tr>
        </tbody>
      </table>
    </div>
  </div>
</template>

<script setup>
defineProps({ orders: { type: Array, default: () => [] } })

function formatDuration(sec) {
  if (!sec) return '0s'
  const h = Math.floor(sec / 3600)
  const m = Math.floor((sec % 3600) / 60)
  const s = sec % 60
  if (h > 0) return `${h}h ${m}m`
  if (m > 0) return `${m}m ${s}s`
  return `${s}s`
}
</script>
