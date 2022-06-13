<template>
  <div id="app">
    <!-- 切换 tab 缓存接口（主要为左侧目录树） -->
    <keep-alive>
      <router-view class="main-width"></router-view>
    </keep-alive>
  </div>
</template>

<script>
export default {
  name: "App",
  components: {
    // AppHeader,
    // AppFooter,
  },
  data() {
    return {};
  },
  mounted() {
    // setInterval(() => {
    //   console.log("scrollHeight:", document.body.scrollHeight);
    //   console.log("clientHeight:", document.body.clientHeight);
    //   console.log("offsetHeight:", document.body.offsetHeight);
    // }, 2000);
  },
};
</script>
