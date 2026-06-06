import { createApp, onMounted, ref } from 'vue'
import axios from 'axios'

const HISTORY_KEY = 'search-engine-history'
const HISTORY_LIMIT = 8

createApp({
  setup() {
    const query = ref('')
    const results = ref([])
    const keywordSuggestions = ref([])
    const searchHistory = ref([])
    const lastSearchedQuery = ref('')
    const loading = ref(false)
    const error = ref('')
    const showSuggestPanel = ref(false)
    const isSuggestLoading = ref(false)

    let debounceTimer = null
    let suggestRequestId = 0
    let blurTimer = null

    const normalizeQuery = () => query.value.trim()

    const formatError = (e, fallback) => {
      if (e.response) {
        return e.response.data?.message || `${fallback}: ${e.response.status}`
      }
      if (e.request) {
        return '网络错误: 无法连接到服务器'
      }
      return `${fallback}: ${e.message}`
    }

    const loadHistory = () => {
      try {
        const raw = window.localStorage.getItem(HISTORY_KEY)
        const parsed = raw ? JSON.parse(raw) : []
        searchHistory.value = Array.isArray(parsed)
          ? parsed.filter(item => typeof item === 'string' && item.trim()).slice(0, HISTORY_LIMIT)
          : []
      } catch (e) {
        console.warn('读取搜索历史失败:', e)
        searchHistory.value = []
      }
    }

    const saveHistory = (value) => {
      const term = value.trim()
      if (!term) return

      const next = [
        term,
        ...searchHistory.value.filter(item => item !== term)
      ].slice(0, HISTORY_LIMIT)

      searchHistory.value = next
      try {
        window.localStorage.setItem(HISTORY_KEY, JSON.stringify(next))
      } catch (e) {
        console.warn('保存搜索历史失败:', e)
      }
    }

    const clearHistory = () => {
      searchHistory.value = []
      window.localStorage.removeItem(HISTORY_KEY)
      showSuggestPanel.value = true
    }

    const fetchSuggestions = async () => {
      const term = normalizeQuery()
      const requestId = ++suggestRequestId

      if (!term) {
        keywordSuggestions.value = []
        isSuggestLoading.value = false
        showSuggestPanel.value = true
        return
      }

      isSuggestLoading.value = true
      showSuggestPanel.value = true

      try {
        const response = await axios.post('/api/keyword', { query: term })
        if (requestId === suggestRequestId) {
          keywordSuggestions.value = response.data.suggestions || []
        }
      } catch (e) {
        if (requestId === suggestRequestId) {
          console.error('关键词推荐错误:', e)
          keywordSuggestions.value = []
        }
      } finally {
        if (requestId === suggestRequestId) {
          isSuggestLoading.value = false
        }
      }
    }

    const onInput = () => {
      clearTimeout(debounceTimer)
      error.value = ''

      if (!normalizeQuery()) {
        ++suggestRequestId
        keywordSuggestions.value = []
        isSuggestLoading.value = false
        showSuggestPanel.value = true
        return
      }

      debounceTimer = setTimeout(fetchSuggestions, 300)
    }

    const search = async () => {
      const term = normalizeQuery()
      if (!term || loading.value) return

      clearTimeout(debounceTimer)
      ++suggestRequestId
      showSuggestPanel.value = false
      isSuggestLoading.value = false
      error.value = ''
      loading.value = true
      lastSearchedQuery.value = term

      try {
        const response = await axios.post('/api/search', { query: term })
        results.value = response.data.results || []
        saveHistory(term)
      } catch (e) {
        console.error('搜索错误:', e)
        error.value = formatError(e, '搜索失败')
      } finally {
        loading.value = false
      }
    }

    const selectSuggestion = (suggestion) => {
      query.value = suggestion
      search()
    }

    const selectHistory = (value) => {
      query.value = value
      search()
    }

    const clearQuery = () => {
      query.value = ''
      keywordSuggestions.value = []
      isSuggestLoading.value = false
      showSuggestPanel.value = true
      ++suggestRequestId
    }

    const onFocus = () => {
      clearTimeout(blurTimer)
      showSuggestPanel.value = true

      if (normalizeQuery() && keywordSuggestions.value.length === 0) {
        clearTimeout(debounceTimer)
        debounceTimer = setTimeout(fetchSuggestions, 300)
      }
    }

    const onBlur = () => {
      blurTimer = setTimeout(() => {
        showSuggestPanel.value = false
      }, 160)
    }

    onMounted(() => {
      loadHistory()
    })

    return {
      query,
      results,
      keywordSuggestions,
      searchHistory,
      lastSearchedQuery,
      loading,
      error,
      showSuggestPanel,
      isSuggestLoading,
      search,
      onInput,
      onFocus,
      onBlur,
      clearQuery,
      clearHistory,
      selectSuggestion,
      selectHistory
    }
  }
}).mount('#app')
