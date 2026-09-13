<template>
  <div class="panel">
    <div class="panel-title">故障设备</div>
    <div class="table-wrapper">
      <table>
        <thead>
          <tr>
            <th>设备编号</th>
            <th>所属电站</th>
            <th>最后心跳</th>
          </tr>
        </thead>
        <tbody>
          <tr v-if="faults.length === 0">
            <td colspan="3" style="color:#8896ab;padding:32px">暂无故障设备</td>
          </tr>
          <tr v-for="f in faults" :key="f.code">
            <td><span class="badge badge-fault">{{ f.code }}</span></td>
            <td>{{ f.station }}</td>
            <td>{{ f.lastSeen || '--' }}</td>
          </tr>
        </tbody>
      </table>
    </div>
  </div>
</template>

<script setup>
defineProps({ faults: { type: Array, default: () => [] } })
</script>
