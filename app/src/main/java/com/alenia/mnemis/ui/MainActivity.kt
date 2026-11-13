package com.alenia.mnemis.ui

import android.Manifest
import android.content.ContentUris
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.provider.MediaStore
import android.view.LayoutInflater
import android.view.View
import android.widget.Toast
import androidx.activity.OnBackPressedCallback
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import androidx.core.view.GravityCompat
import androidx.core.widget.doAfterTextChanged
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.GridLayoutManager
import androidx.recyclerview.widget.LinearLayoutManager
import com.alenia.mnemis.R
import com.alenia.mnemis.databinding.ActivityMainBinding
import com.alenia.mnemis.databinding.DialogOpenUrlBinding
import com.alenia.mnemis.model.PluginItem
import com.alenia.mnemis.model.VideoAlbum
import com.alenia.mnemis.model.VideoItem
import com.alenia.mnemis.plugin.MnemisPluginRegistry
import com.alenia.mnemis.prefs.MnemisPreferences
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class MainActivity : AppCompatActivity() {

    private enum class Section {
        HOME, VIDEOS, WEB, PLUGINS, DOWNLOADS, SETTINGS
    }

    private lateinit var binding: ActivityMainBinding
    private lateinit var prefs: MnemisPreferences

    private var currentSection = Section.HOME
    private var allVideos: List<VideoItem> = emptyList()
    private var selectedAlbumName: String? = null
    private var currentQuery: String = ""

    private val albumAdapter = AlbumAdapter { album ->
        selectedAlbumName = album.name
        applySearchFilter()
    }

    private val videoAdapter = VideoAdapter { videoItem ->
        openPlayer(videoItem.uri.toString(), videoItem.title)
    }

    private val downloadsAdapter = VideoAdapter { videoItem ->
        openPlayer(videoItem.uri.toString(), videoItem.title)
    }

    private lateinit var pluginAdapter: PluginAdapter

    private val permissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { isGranted ->
        if (isGranted) {
            loadLocalVideos()
        } else {
            showPermissionDeniedState()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        prefs = MnemisPreferences(this)

        setupDrawerAndToolbar()
        setupHomeSection()
        setupVideosSection()
        setupWebSection()
        setupPluginsSection()
        setupDownloadsSection()
        setupSettingsSection()
        setupBackNavigation()

        switchSection(Section.HOME)
        handleIncomingIntent(intent)
        checkPermissionAndLoadVideos()
    }

    override fun onResume() {
        super.onResume()
        refreshHomeCard()
    }

    private fun setupDrawerAndToolbar() {
        binding.btnMenu.setOnClickListener {
            binding.drawerLayout.openDrawer(GravityCompat.START)
        }

        binding.navigationView.setCheckedItem(R.id.nav_home)
        binding.navigationView.setNavigationItemSelectedListener { item ->
            when (item.itemId) {
                R.id.nav_home -> switchSection(Section.HOME)
                R.id.nav_videos -> switchSection(Section.VIDEOS)
                R.id.nav_web -> switchSection(Section.WEB)
                R.id.nav_plugins -> switchSection(Section.PLUGINS)
                R.id.nav_downloads -> switchSection(Section.DOWNLOADS)
                R.id.nav_settings -> switchSection(Section.SETTINGS)
            }
            binding.drawerLayout.closeDrawer(GravityCompat.START)
            true
        }

        binding.btnSearchToggle.setOnClickListener {
            val isVisible = binding.etSearchBar.visibility == View.VISIBLE
            if (isVisible) {
                binding.etSearchBar.setText("")
                binding.etSearchBar.visibility = View.GONE
                binding.tvToolbarTitle.visibility = View.VISIBLE
            } else {
                binding.tvToolbarTitle.visibility = View.GONE
                binding.etSearchBar.visibility = View.VISIBLE
                binding.etSearchBar.requestFocus()
            }
        }

        binding.etSearchBar.doAfterTextChanged { editable ->
            currentQuery = editable?.toString()?.trim().orEmpty()
            applySearchFilter()
        }

        binding.fabOpenLink.setOnClickListener {
            showOpenUrlDialog()
        }
    }

    private fun switchSection(section: Section) {
        currentSection = section
        binding.sectionHome.visibility = if (section == Section.HOME) View.VISIBLE else View.GONE
        binding.sectionVideos.visibility = if (section == Section.VIDEOS) View.VISIBLE else View.GONE
        binding.sectionWeb.visibility = if (section == Section.WEB) View.VISIBLE else View.GONE
        binding.sectionPlugins.visibility = if (section == Section.PLUGINS) View.VISIBLE else View.GONE
        binding.sectionDownloads.visibility = if (section == Section.DOWNLOADS) View.VISIBLE else View.GONE
        binding.sectionSettings.visibility = if (section == Section.SETTINGS) View.VISIBLE else View.GONE

        val showFab = section == Section.HOME || section == Section.VIDEOS || section == Section.DOWNLOADS
        binding.fabOpenLink.visibility = if (showFab) View.VISIBLE else View.GONE

        val showSearch = section == Section.VIDEOS || section == Section.PLUGINS || section == Section.DOWNLOADS
        binding.btnSearchToggle.visibility = if (showSearch) View.VISIBLE else View.GONE
        if (!showSearch) {
            binding.etSearchBar.setText("")
            binding.etSearchBar.visibility = View.GONE
            binding.tvToolbarTitle.visibility = View.VISIBLE
        }

        binding.tvToolbarTitle.text = when (section) {
            Section.HOME -> getString(R.string.nav_home)
            Section.VIDEOS -> getString(R.string.nav_videos)
            Section.WEB -> getString(R.string.nav_web)
            Section.PLUGINS -> getString(R.string.nav_plugins)
            Section.DOWNLOADS -> getString(R.string.nav_downloads)
            Section.SETTINGS -> getString(R.string.nav_settings)
        }

        if (section == Section.HOME) {
            refreshHomeCard()
        } else if (section == Section.SETTINGS) {
            loadSettingsIntoUi()
        } else if (section == Section.PLUGINS) {
            refreshPluginsList()
        }
    }

    private fun setupHomeSection() {
        binding.btnHomeOpenWeb.setOnClickListener {
            binding.navigationView.setCheckedItem(R.id.nav_web)
            switchSection(Section.WEB)
        }
        binding.btnHomeMyVideos.setOnClickListener {
            binding.navigationView.setCheckedItem(R.id.nav_videos)
            switchSection(Section.VIDEOS)
        }
        binding.btnHomePlugins.setOnClickListener {
            binding.navigationView.setCheckedItem(R.id.nav_plugins)
            switchSection(Section.PLUGINS)
        }
        refreshHomeCard()
    }

    private fun refreshHomeCard() {
        val last = prefs.getLastPlayed()
        if (last == null) {
            binding.cardLastPlayed.visibility = View.GONE
            binding.tvHomeEmpty.visibility = View.VISIBLE
            return
        }
        binding.tvHomeEmpty.visibility = View.GONE
        binding.cardLastPlayed.visibility = View.VISIBLE
        binding.tvLastPlayedTitle.text = last.title
        binding.tvLastPlayedType.text = if (last.isWeb) {
            getString(R.string.home_type_web)
        } else {
            getString(R.string.home_type_local)
        }
        binding.btnResumeLastPlayed.setOnClickListener {
            if (last.isWeb) {
                openWebPlayer(last.source, last.title)
            } else {
                openPlayer(last.source, last.title)
            }
        }
    }

    private fun setupVideosSection() {
        binding.recyclerViewAlbums.apply {
            layoutManager = GridLayoutManager(this@MainActivity, 2)
            adapter = albumAdapter
        }

        binding.recyclerViewVideos.apply {
            layoutManager = LinearLayoutManager(this@MainActivity)
            adapter = videoAdapter
        }

        binding.btnBackToAlbums.setOnClickListener {
            selectedAlbumName = null
            applySearchFilter()
        }

        binding.sectionVideos.setOnRefreshListener {
            loadLocalVideos()
        }

        binding.btnGrantPermission.setOnClickListener {
            requestStoragePermission()
        }
    }

    private fun setupWebSection() {
        binding.btnLaunchWebUrl.setOnClickListener {
            val rawUrl = binding.etWebSectionUrl.text?.toString()?.trim().orEmpty()
            if (rawUrl.isEmpty()) {
                Toast.makeText(this, "Ingresa una URL válida", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }
            val normalized = if (rawUrl.startsWith("http://") || rawUrl.startsWith("https://")) {
                rawUrl
            } else {
                "https://$rawUrl"
            }
            openWebPlayer(normalized, "Web Player")
        }

        binding.btnQuickYouTube.setOnClickListener {
            openWebPlayer("https://m.youtube.com", "YouTube")
        }

        binding.btnQuickTwitch.setOnClickListener {
            openWebPlayer("https://m.twitch.tv", "Twitch")
        }
    }

    private fun setupPluginsSection() {
        pluginAdapter = PluginAdapter(
            onToggleEnabled = { plugin, enabled ->
                prefs.setPluginEnabled(plugin.id, enabled)
                refreshPluginsList()
            },
            onInstallClick = { plugin ->
                prefs.setPluginInstalled(plugin.id, true)
                prefs.setPluginEnabled(plugin.id, true)
                Toast.makeText(this, "${plugin.name} descargado y activado", Toast.LENGTH_SHORT).show()
                refreshPluginsList()
            }
        )

        binding.recyclerViewPlugins.apply {
            layoutManager = LinearLayoutManager(this@MainActivity)
            adapter = pluginAdapter
        }
        refreshPluginsList()
    }

    private fun refreshPluginsList() {
        val registered = MnemisPluginRegistry.allPlugins().map { plugin ->
            PluginItem(
                id = plugin.id,
                name = plugin.name,
                version = plugin.version,
                description = plugin.description,
                isBuiltIn = true,
                isInstalled = true,
                isEnabled = prefs.isPluginEnabled(plugin.id)
            )
        }
        val optionalCatalog = listOf(
            PluginItem(
                id = "generic_hls_sniffer",
                name = "Stream Sniffer",
                version = "v0.9.0",
                description = getString(R.string.plugin_desc_hls_sniffer),
                isBuiltIn = false,
                isInstalled = prefs.isPluginInstalled("generic_hls_sniffer"),
                isEnabled = prefs.isPluginEnabled("generic_hls_sniffer")
            ),
            PluginItem(
                id = "subtitle_overlay",
                name = "Subtitles SRT/VTT",
                version = "v0.8.2",
                description = getString(R.string.plugin_desc_subtitles),
                isBuiltIn = false,
                isInstalled = prefs.isPluginInstalled("subtitle_overlay"),
                isEnabled = prefs.isPluginEnabled("subtitle_overlay")
            )
        )
        val catalog = registered + optionalCatalog

        val filtered = if (currentQuery.isEmpty()) {
            catalog
        } else {
            catalog.filter {
                it.name.contains(currentQuery, ignoreCase = true) ||
                    it.description.contains(currentQuery, ignoreCase = true)
            }
        }
        pluginAdapter.submitList(filtered)
    }

    private fun setupDownloadsSection() {
        binding.recyclerViewDownloads.apply {
            layoutManager = LinearLayoutManager(this@MainActivity)
            adapter = downloadsAdapter
        }
    }

    private fun setupSettingsSection() {
        loadSettingsIntoUi()
        binding.btnSaveSettings.setOnClickListener {
            prefs.defaultUsername = binding.etSettingsUsername.text?.toString().orEmpty()
            prefs.relayServerUrl = binding.etSettingsRelayUrl.text?.toString().orEmpty()
            prefs.adBlockEnabled = binding.switchSettingsAdBlock.isChecked
            prefs.koalaSyncPluginEnabled = binding.switchSettingsKoalaSync.isChecked
            Toast.makeText(this, "Ajustes guardados", Toast.LENGTH_SHORT).show()
        }
    }

    private fun loadSettingsIntoUi() {
        binding.etSettingsUsername.setText(prefs.defaultUsername)
        binding.etSettingsRelayUrl.setText(prefs.relayServerUrl)
        binding.switchSettingsAdBlock.isChecked = prefs.adBlockEnabled
        binding.switchSettingsKoalaSync.isChecked = prefs.koalaSyncPluginEnabled
    }

    private fun setupBackNavigation() {
        onBackPressedDispatcher.addCallback(this, object : OnBackPressedCallback(true) {
            override fun handleOnBackPressed() {
                when {
                    binding.drawerLayout.isDrawerOpen(GravityCompat.START) -> {
                        binding.drawerLayout.closeDrawer(GravityCompat.START)
                    }
                    binding.etSearchBar.visibility == View.VISIBLE -> {
                        binding.etSearchBar.setText("")
                        binding.etSearchBar.visibility = View.GONE
                        binding.tvToolbarTitle.visibility = View.VISIBLE
                    }
                    currentSection == Section.VIDEOS && selectedAlbumName != null -> {
                        selectedAlbumName = null
                        applySearchFilter()
                    }
                    currentSection != Section.HOME -> {
                        binding.navigationView.setCheckedItem(R.id.nav_home)
                        switchSection(Section.HOME)
                    }
                    else -> {
                        finish()
                    }
                }
            }
        })
    }

    private fun checkPermissionAndLoadVideos() {
        val permission = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            Manifest.permission.READ_MEDIA_VIDEO
        } else {
            Manifest.permission.READ_EXTERNAL_STORAGE
        }

        if (ContextCompat.checkSelfPermission(this, permission) == PackageManager.PERMISSION_GRANTED) {
            loadLocalVideos()
        } else {
            permissionLauncher.launch(permission)
        }
    }

    private fun requestStoragePermission() {
        val permission = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            Manifest.permission.READ_MEDIA_VIDEO
        } else {
            Manifest.permission.READ_EXTERNAL_STORAGE
        }
        permissionLauncher.launch(permission)
    }

    private fun loadLocalVideos() {
        binding.progressBar.visibility = View.VISIBLE
        binding.layoutEmpty.visibility = View.GONE

        lifecycleScope.launch(Dispatchers.IO) {
            val videos = queryMediaStoreVideos()
            withContext(Dispatchers.Main) {
                binding.progressBar.visibility = View.GONE
                binding.sectionVideos.isRefreshing = false
                allVideos = videos
                applySearchFilter()
            }
        }
    }

    private fun applySearchFilter() {
        val searchFiltered = if (currentQuery.isEmpty()) {
            allVideos
        } else {
            allVideos.filter { it.title.contains(currentQuery, ignoreCase = true) }
        }

        val activeAlbum = selectedAlbumName
        if (activeAlbum == null && currentQuery.isEmpty()) {
            val albums = allVideos
                .groupBy { it.bucketName }
                .map { (name, items) -> VideoAlbum(name = name, coverUri = items.first().uri, videos = items) }
                .sortedByDescending { it.videos.size }

            binding.layoutAlbumHeader.visibility = View.GONE
            binding.recyclerViewAlbums.visibility = if (albums.isEmpty()) View.GONE else View.VISIBLE
            binding.recyclerViewVideos.visibility = View.GONE
            albumAdapter.submitList(albums)

            if (albums.isEmpty()) {
                binding.layoutEmpty.visibility = View.VISIBLE
                binding.tvEmptyMessage.text = getString(R.string.no_videos_found)
                binding.btnGrantPermission.visibility = View.GONE
            } else {
                binding.layoutEmpty.visibility = View.GONE
            }
        } else {
            val visibleVideos = if (activeAlbum != null) {
                searchFiltered.filter { it.bucketName == activeAlbum }
            } else {
                searchFiltered
            }

            binding.layoutAlbumHeader.visibility = if (activeAlbum != null) View.VISIBLE else View.GONE
            binding.tvCurrentAlbumName.text = activeAlbum.orEmpty()
            binding.recyclerViewAlbums.visibility = View.GONE
            binding.recyclerViewVideos.visibility = if (visibleVideos.isEmpty()) View.GONE else View.VISIBLE
            videoAdapter.submitList(visibleVideos)

            if (visibleVideos.isEmpty()) {
                binding.layoutEmpty.visibility = View.VISIBLE
                binding.tvEmptyMessage.text = if (currentQuery.isNotEmpty()) {
                    getString(R.string.search_no_results, currentQuery)
                } else {
                    getString(R.string.no_videos_found)
                }
                binding.btnGrantPermission.visibility = View.GONE
            } else {
                binding.layoutEmpty.visibility = View.GONE
            }
        }

        val downloadedVideos = searchFiltered.filter {
            it.path.contains("/Download", ignoreCase = true)
        }
        downloadsAdapter.submitList(downloadedVideos)
        binding.tvEmptyDownloads.visibility = if (downloadedVideos.isEmpty()) View.VISIBLE else View.GONE

        if (currentSection == Section.PLUGINS) {
            refreshPluginsList()
        }
    }

    private fun queryMediaStoreVideos(): List<VideoItem> {
        val list = mutableListOf<VideoItem>()
        val projection = arrayOf(
            MediaStore.Video.Media._ID,
            MediaStore.Video.Media.TITLE,
            MediaStore.Video.Media.DURATION,
            MediaStore.Video.Media.SIZE,
            MediaStore.Video.Media.WIDTH,
            MediaStore.Video.Media.HEIGHT,
            MediaStore.Video.Media.DATA,
            MediaStore.Video.Media.BUCKET_DISPLAY_NAME
        )

        val sortOrder = "${MediaStore.Video.Media.DATE_ADDED} DESC"

        try {
            contentResolver.query(
                MediaStore.Video.Media.EXTERNAL_CONTENT_URI,
                projection,
                null,
                null,
                sortOrder
            )?.use { cursor ->
                val idColumn = cursor.getColumnIndexOrThrow(MediaStore.Video.Media._ID)
                val titleColumn = cursor.getColumnIndexOrThrow(MediaStore.Video.Media.TITLE)
                val durationColumn = cursor.getColumnIndexOrThrow(MediaStore.Video.Media.DURATION)
                val sizeColumn = cursor.getColumnIndexOrThrow(MediaStore.Video.Media.SIZE)
                val widthColumn = cursor.getColumnIndexOrThrow(MediaStore.Video.Media.WIDTH)
                val heightColumn = cursor.getColumnIndexOrThrow(MediaStore.Video.Media.HEIGHT)
                val dataColumn = cursor.getColumnIndexOrThrow(MediaStore.Video.Media.DATA)
                val bucketColumn = cursor.getColumnIndex(MediaStore.Video.Media.BUCKET_DISPLAY_NAME)

                while (cursor.moveToNext()) {
                    val id = cursor.getLong(idColumn)
                    val title = cursor.getString(titleColumn) ?: "Video"
                    val duration = cursor.getLong(durationColumn)
                    val size = cursor.getLong(sizeColumn)
                    val width = cursor.getInt(widthColumn)
                    val height = cursor.getInt(heightColumn)
                    val path = cursor.getString(dataColumn) ?: ""
                    val bucketName = if (bucketColumn >= 0) {
                        cursor.getString(bucketColumn)?.takeIf { it.isNotBlank() } ?: "Videos"
                    } else {
                        "Videos"
                    }

                    val contentUri = ContentUris.withAppendedId(
                        MediaStore.Video.Media.EXTERNAL_CONTENT_URI,
                        id
                    )

                    val resolution = if (width > 0 && height > 0) "${width}x${height}" else ""

                    list.add(VideoItem(id, contentUri, title, duration, size, resolution, path, bucketName))
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }

        return list
    }

    private fun showPermissionDeniedState() {
        binding.layoutEmpty.visibility = View.VISIBLE
        binding.tvEmptyMessage.text = "Se requiere permiso para leer los videos de tu dispositivo."
        binding.btnGrantPermission.visibility = View.VISIBLE
    }

    private fun showOpenUrlDialog() {
        val dialogBinding = DialogOpenUrlBinding.inflate(LayoutInflater.from(this))
        val dialog = AlertDialog.Builder(this)
            .setView(dialogBinding.root)
            .setCancelable(true)
            .create()

        dialogBinding.btnCancelDialog.setOnClickListener {
            dialog.dismiss()
        }

        dialogBinding.btnPlayDialog.setOnClickListener {
            val url = dialogBinding.etUrl.text?.toString()?.trim()
            if (url.isNullOrEmpty()) {
                dialogBinding.tilUrl.error = "Por favor ingresa una URL válida"
                return@setOnClickListener
            }

            dialogBinding.tilUrl.error = null
            dialog.dismiss()
            openWebPlayer(url, "Web Player")
        }

        dialog.show()
    }

    private fun openPlayer(videoUriString: String, title: String) {
        val intent = Intent(this, PlayerActivity::class.java).apply {
            putExtra(PlayerActivity.EXTRA_VIDEO_URI, videoUriString)
            putExtra(PlayerActivity.EXTRA_VIDEO_TITLE, title)
        }
        startActivity(intent)
    }

    private fun openWebPlayer(webUrl: String, title: String) {
        val intent = Intent(this, WebPlayerActivity::class.java).apply {
            putExtra(WebPlayerActivity.EXTRA_WEB_URL, webUrl)
            putExtra(WebPlayerActivity.EXTRA_WEB_TITLE, title)
        }
        startActivity(intent)
    }

    private fun handleIncomingIntent(intent: Intent?) {
        if (intent == null) return
        val action = intent.action
        val data = intent.data

        if (Intent.ACTION_VIEW == action && data != null) {
            val uriStr = data.toString()
            if (uriStr.startsWith("http://") || uriStr.startsWith("https://")) {
                openWebPlayer(uriStr, "Web Player")
            } else {
                openPlayer(uriStr, "Video")
            }
        }
    }
}
