import { createApp, ref } from 'vue'
import axios from 'axios'

createApp({
  setup() {
    const query = ref('')
    const searchType = ref('web')
    const results = ref([])
    const suggestions = ref([])
    const loading = ref(false)
    const error = ref('')
    let debounceTimer = null

    const search = async () => {
      if (!query.value.trim()) return
      
      loading.value = true
      error.value = ''
      results.value = []
      suggestions.value = []

      try {
        if (searchType.value === 'web') {
          const response = await axios.post('/api/search', {
            query: query.value
          })
          results.value = response.data.results
        } else {
          const response = await axios.post('/api/keyword', {
            query: query.value
          })
          suggestions.value = response.data.suggestions
        }
      } catch (e) {
        console.error('搜索错误:', e)
        if (e.response) {
          error.value = e.response.data?.message || '服务器错误: ' + e.response.status
        } else if (e.request) {
          error.value = '网络错误: 无法连接到服务器'
        } else {
          error.value = '请求失败: ' + e.message
        }
      } finally {
        loading.value = false
      }
    }

    const onInput = () => {
      // 关键词推荐：用户输入时实时推荐
      if (searchType.value === 'keyword' && query.value.trim()) {
        clearTimeout(debounceTimer)
        debounceTimer = setTimeout(() => {
          search()
        }, 300)
      }
    }

    const clickSuggestion = (suggestion) => {
      query.value = suggestion
      searchType.value = 'web'
      search()
    }

    return {
      query,
      searchType,
      results,
      suggestions,
      loading,
      error,
      search,
      onInput,
      clickSuggestion
    }
  }
}).mount('#app')
