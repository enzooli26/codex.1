<template>
  <div :class="['stat-card', color]">
    <span class="label">{{ label }}</span>
    <span class="value">{{ formatted }}</span>
  </div>
</template>

<script setup>
import { computed } from 'vue'

const props = defineProps({
  label: String,
  value: { type: [Number, String], default: 0 },
  color: { type: String, default: 'blue' },
  prefix: { type: String, default: '' },
  decimals: { type: Number, default: 0 }
})

const formatted = computed(() => {
  const v = Number(props.value) || 0
  if (props.prefix === '¥') {
    return `¥${v.toFixed(2)}`
  }
  if (props.decimals > 0) {
    return `${props.prefix}${v.toFixed(props.decimals)}`
  }
  return `${props.prefix}${v.toLocaleString()}`
})
</script>
