require "bundler/gem_tasks"
require "rake/extensiontask"
require "rspec/core/rake_task"

DICT_SOURCE = 'mecab-naist-jdic-0.6.3b-20111013.tar.gz'

def spec_dir
  Pathname(__FILE__).join('..', 'spec')
end

task :default => :spec
task :spec => [ :compile, 'spec:setup' ]

Rake::ExtensionTask.new 'mecaby' do |ext|
  ext.lib_dir = 'lib/mecaby'
end

RSpec::Core::RakeTask.new(:spec)

namespace :spec do
  namespace :dictionary do
    def dict_dir
      @dict_dir ||= spec_dir.join('dict')
    end

    def build_dir
      @build_dir ||= dict_dir.join('.build')
    end

    def setup_dictionary(charset)
      dest_dir = dict_dir.join(charset)
      return if dest_dir.join('dicrc').file?

      build_dir.mkdir unless build_dir.directory?
      system 'tar', 'xzf', spec_dir.join('dict', DICT_SOURCE).to_s, '-C', build_dir.to_s
      Dir.chdir build_dir.join(File.basename(DICT_SOURCE, '.tar.gz')).to_s do
        system './configure', "--with-dicdir=#{dest_dir.to_s}", "--with-charset=#{charset}"
        system 'make', 'install-dicDATA'
        system 'make', 'clean'
      end
    end

    task :setup_utf8 do
      setup_dictionary('utf-8')
    end

    task :setup_sjis do
      setup_dictionary('sjis')
    end

    task :setup_euc do
      setup_dictionary('euc-jp')
    end

    task :clean do
      system "rm -rf #{spec_dir.join('dict', 'utf-8')}"
      system "rm -rf #{spec_dir.join('dict', 'sjis')}"
      system "rm -rf #{spec_dir.join('dict', 'euc-jp')}"
    end

    task :setup => %w[
      spec:dictionary:setup_utf8
      spec:dictionary:setup_sjis
      spec:dictionary:setup_euc
    ]
  end

  task :setup => ['spec:dictionary:setup']
end
