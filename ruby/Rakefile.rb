require 'rake'
require 'fileutils'
require 'open-uri'
require 'uri'
require 'pathname'

desc "Generate additional C/C++ source files"
task :generate_sources do
    # Define versions and URLs for dependencies
    mcpp_version = "2.7.2.18"
    mcpp_url = "https://github.com/zeroc-ice/mcpp/archive/refs/tags/v#{mcpp_version}.tar.gz"
    mcpp_local_filename = "dist/mcpp-#{mcpp_version}.tar.gz"
    dist_dir = 'dist'

    unless File.exist?(dist_dir)
        FileUtils.mkdir_p(dist_dir)
    end

    unless File.exist?(mcpp_local_filename)
        puts "Downloading #{mcpp_url}..."
        URI.open(mcpp_url) do |response|
            File.open(mcpp_local_filename, 'wb') do |file|
                file.write(response.read)
            end
        end
        puts "Download complete."

        puts "Extracting #{mcpp_local_filename}..."
        FileUtils.mkdir_p(dist_dir)
        system("tar -xzf #{mcpp_local_filename} -C #{dist_dir}")
        puts "Extraction complete."
    end

    # MCPP sources
    Dir.foreach("dist/mcpp-#{mcpp_version}") do |relative_path|
        source_file = "dist/mcpp-#{mcpp_version}/#{relative_path}"
        if File.file?(source_file) && ['.c', '.h'].include?(File.extname(source_file).downcase)
            script_directory = File.dirname(File.absolute_path(__FILE__))
            target_file = File.join(script_directory, 'dist/ice/mcpp', relative_path)
            target_dir = File.dirname(target_file)

            unless File.exist?(target_dir)
                FileUtils.mkdir_p(target_dir)
            end
            FileUtils.cp(source_file, target_file)
        end
    end

    # Ensure C++ srcs  have been build
    system('make srcs OPTIMIZE=yes -C ../cpp')

    # Define source directories for Ice C++
    ice_cpp_sources = [
        "../cpp/src/Ice",
        "../cpp/src/Ice/generated",
        "../cpp/src/Ice/SSL",
        "../cpp/src/IceDiscovery",
        "../cpp/src/IceDiscovery/generated",
        "../cpp/src/IceLocatorDiscovery",
        "../cpp/src/IceLocatorDiscovery/generated",
        "../cpp/src/slice2rb",
        "../cpp/src/Slice",
        "../cpp/include/Ice",
        "../cpp/include/Ice/SSL",
        "../cpp/include/generated/Ice"]

    for source_dir in ice_cpp_sources do
        Dir.foreach(source_dir) do |entry|
            source_file = "#{source_dir}/#{entry}"
            if File.file?(source_file) && ['.c', '.cpp', '.h'].include?(File.extname(source_file).downcase)
                relative_path = Pathname.new(File.expand_path(source_file)).relative_path_from(Pathname.new(File.expand_path('..'))).to_s
                script_directory = File.dirname(File.absolute_path(__FILE__))
                target_file = File.join(script_directory, 'dist/ice', relative_path)
                target_dir = File.dirname(target_file)

                unless File.exist?(target_dir)
                    FileUtils.mkdir_p(target_dir)
                end
                FileUtils.cp(source_file, target_file)
            end
        end
    end

    # Ruby native extension sources
    Dir.foreach("src/IceRuby") do |relative_path|
        source_file = "src/IceRuby/#{relative_path}"
        if File.file?(source_file) && ['.cpp', '.h'].include?(File.extname(source_file).downcase)
            script_directory = File.dirname(File.absolute_path(__FILE__))
            target_file = File.join(script_directory, 'dist/IceRuby', relative_path)
            target_dir = File.dirname(target_file)

            unless File.exist?(target_dir)
                FileUtils.mkdir_p(target_dir)
            end
            FileUtils.cp(source_file, target_file)
        end
    end

    # Ruby sources
    system('make srcs OPTIMIZE=yes')
    ice_ruby_sources = ["ruby", "ruby/Ice"]
    for source_dir in ice_ruby_sources do
        Dir.foreach(source_dir) do |entry|
            source_file = "#{source_dir}/#{entry}"
            if File.file?(source_file) && ['.rb'].include?(File.extname(source_file).downcase)
                relative_path = Pathname.new(File.expand_path(source_file)).relative_path_from(
                    Pathname.new(File.expand_path('./ruby'))).to_s
                script_directory = File.dirname(File.absolute_path(__FILE__))
                target_file = File.join(script_directory, 'dist/lib', relative_path)
                target_dir = File.dirname(target_file)

                unless File.exist?(target_dir)
                    FileUtils.mkdir_p(target_dir)
                end
                FileUtils.cp(source_file, target_file)
            end
        end
    end
end

desc "Build the gem"
task :build => 'generate_sources' do
    sh "gem build ice.gemspec"
end

task :default => [:build]
