<template>
  <div class="dashboard">
    <div class="connection-bar">
      <div class="connection-status">
        <span class="connection-dot" :class="dotClass"></span>
        <span>{{ statusText }}</span>
      </div>
      <button v-if="!connected" class="reconnect-btn" @click="onReconnect">重新连接</button>
    </div>

    <div class="card-row">
      <StatCard label="今日营收" :value="summary.todayRevenue || 0" color="blue" prefix="¥" :decimals="2" />
      <StatCard label="本月营收" :value="summary.monthRevenue || 0" color="green" prefix="¥" :decimals="2" />
      <StatCard label="累计营收" :value="summary.revenue || 0" color="purple" prefix="¥" :decimals="2" />
      <StatCard label="今日订单" :value="summary.todayOrders || 0" color="orange" />
      <StatCard label="注册用户" :value="summary.users || 0" color="red" />
    </div>

    <div class="chart-row">
      <RevenueTrendChart :trend="summary.revenueTrend || []" />
      <ChargerStatusPie
        :idle="summary.idle || 0"
        :charging="summary.charging || 0"
        :fault="summary.fault || 0"
        :total="summary.chargers || 0"
      />
    </div>

    <div class="panel panel-full">
      <StationRankingBar :ranking="summary.stationRanking || []" />
    </div>

    <div class="chart-row">
      <ActiveOrdersTable :orders="activeOrders" />
      <FaultChargersTable :faults="summary.faultChargers || []" />
    </div>

    <div class="panel-title" style="padding:0 4px">实时遥测数据</div>
    <TelemetryCharts :chargers="chargers" />
  </div>
</template>

<script setup>
import { computed } from 'vue'
import StatCard from '../components/StatCard.vue'
import RevenueTrendChart from '../components/RevenueTrendChart.vue'
import ChargerStatusPie from '../components/ChargerStatusPie.vue'
import StationRankingBar from '../components/StationRankingBar.vue'
import ActiveOrdersTable from '../components/ActiveOrdersTable.vue'
import FaultChargersTable from '../components/FaultChargersTable.vue'
import TelemetryCharts from '../components/TelemetryCharts.vue'

const props = defineProps({
  summary: { type: Object, default: () => ({}) },
  chargers: { type: Array, default: () => [] },
  orders: { type: Array, default: () => [] },
  connected: { type: Boolean, default: false },
  backendStatus: { type: String, default: 'disconnected' },
  onReconnect: { type: Function, default: () => {} }
})

const activeOrders = computed(() => {
  return (props.orders || []).filter(o => o.status === 'CHARGING')
})

const statusText = computed(() => {
  if (!props.connected) return 'WebSocket 未连接'
  switch (props.backendStatus) {
    case 'ready': return '已连接'
    case 'connected': return '已连接服务器，等待认证...'
    case 'connecting': return '正在连接服务器...'
    case 'auth_failed': return '管理员认证失败，请检查配置'
    default: return '后端服务器未连接'
  }
})

const dotClass = computed(() => {
  if (!props.connected) return 'offline'
  if (props.backendStatus === 'ready') return 'online'
  if (props.backendStatus === 'auth_failed') return 'offline'
  return 'warning'
})
</script>

<style scoped>
.connection-bar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

.connection-status {
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 14px;
  font-weight: 500;
}

.connection-dot {
  width: 10px;
  height: 10px;
  border-radius: 50%;
  flex-shrink: 0;
}

.connection-dot.online { background: #3ddc97; }
.connection-dot.offline { background: #ff6b6b; }
.connection-dot.warning { background: #f59e0b; }

.reconnect-btn {
  background: #3f6fe5;
  color: #fff;
  border: none;
  border-radius: 8px;
  padding: 8px 20px;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition: background 0.2s;
}

.reconnect-btn:hover {
  background: #315fd1;
}

.reconnect-btn:active {
  background: #254dac;
}
</style>
