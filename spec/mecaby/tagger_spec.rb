require 'spec_helper'

module Mecaby
  describe Tagger do
    subject(:tagger) {
      described_class.new([
        "-d #{dict_dir.join('utf-8')}",
        *additional_args
      ])
    }

    let(:additional_args) { [] }

    describe '.new' do
      context 'When the subject method is called with non-existing dictionary path' do
        subject(:tagger) do
          described_class.new("-d #{dict_dir.join('non-existing-dict')}")
        end

        it 'raises Mecaby::DictionaryNotFound' do
          expect { subject }.to raise_error(Mecaby::DictionaryNotFound)
        end
      end
    end

    describe '#parse' do
      context 'When the tagger is created with "-Oyomi"' do
        let(:additional_args) { [ '-Oyomi' ] }

        context 'the subject method is called with "太郎と花子"' do
          let(:input) { "太郎と花子" }
          subject { tagger.parse(input) }

          it { should eq("タロウトハナコ\n") }
        end
      end

      context 'When the tagger is created with "-Owakati"' do
        let(:additional_args) { [ '-Owakati' ] }

        context 'the subject method is called with "太郎と花子"' do
          let(:input) { "太郎と花子" }
          subject { tagger.parse(input) }

          it { should eq("太郎 と 花子 \n") }
        end
      end
    end

    describe '#nbest_parse' do
      context 'When the tagger is created without -l option' do
        context 'the subject method is called with 3 and "太郎と花子"' do
          let(:input) { "太郎と花子" }
          subject { tagger.nbest_parse(3, input) }

          it 'raises Mecaby::Error' do
            expect { subject }.to raise_error(Mecaby::Error)
          end
        end
      end

      context 'When the tagger is created with "-l 1"' do
        let(:additional_args) { [ '-l', '1' ] }

        context 'the subject method is called with 3 and "太郎と花子"' do
          let(:input) { "太郎と花子" }
          subject { tagger.nbest_parse(3, input) }

          it 'includes three results' do
            expect(subject.lines.select {|l| l == "EOS\n" }.count).to eq(3)
          end
        end
      end

    end
  end
end
